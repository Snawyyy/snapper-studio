#include "document/ChannelJson.h"

#include <QJsonArray>

#include <algorithm>
#include <array>
#include <cmath>

#include "eval/Curves.h"

namespace snapper {

namespace {

constexpr double kMaxCoordinate = 1e6;
constexpr double kMaxScale = 1e3;
constexpr int kMaxFrame = 100000;

struct PropertyName {
  ChannelProperty property;
  const char* name;
};

// In the order of ChannelProperty.
constexpr std::array<PropertyName, kChannelPropertyCount> kPropertyNames = {{
    {ChannelProperty::kPointPosition, "point"},
    {ChannelProperty::kLayerPosition, "position"},
    {ChannelProperty::kLayerAngle, "angle"},
    {ChannelProperty::kLayerScale, "scale"},
    {ChannelProperty::kBonePosition, "bone_position"},
    {ChannelProperty::kBoneAngle, "bone_angle"},
    {ChannelProperty::kBoneScale, "bone_scale"},
}};

std::optional<ChannelProperty> PropertyFrom(const QString& name) {
  for (const PropertyName& entry : kPropertyNames) {
    if (name == QLatin1String(entry.name)) {
      return entry.property;
    }
  }
  return std::nullopt;
}

double Finite(const QJsonValue& value, double fallback, double limit) {
  const double number = value.toDouble(fallback);
  return std::isfinite(number) ? std::clamp(number, -limit, limit) : fallback;
}

}  // namespace

QJsonObject ChannelJson::WriteChannel(const Channel& channel) {
  const bool is_single = channel.property == ChannelProperty::kLayerAngle;
  QJsonArray keys;
  for (const Key& key : channel.keys) {
    QJsonObject json{{QStringLiteral("t"), key.frame},
                     {QStringLiteral("curve"), Curves::FileName(key.curve)}};
    if (is_single) {
      json.insert(QStringLiteral("v"), key.value.x());
    } else {
      json.insert(QStringLiteral("x"), key.value.x());
      json.insert(QStringLiteral("y"), key.value.y());
    }
    if (key.param != 0) {
      json.insert(QStringLiteral("param"), key.param);
    }
    keys.append(json);
  }
  return {{QStringLiteral("target"), channel.target},
          {QStringLiteral("property"),
           QString::fromLatin1(
               kPropertyNames[static_cast<size_t>(channel.property)].name)},
          {QStringLiteral("keys"), keys}};
}

void ChannelJson::WriteTransform(const Layer& layer, QJsonObject& json) {
  const bool has_origin = !layer.origin.isNull();
  if (has_origin) {
    json.insert(QStringLiteral("origin"),
                QJsonArray{layer.origin.x(), layer.origin.y()});
  }
  const LayerTransform& transform = layer.transform;
  if (transform == LayerTransform{}) {
    return;
  }
  json.insert(QStringLiteral("transform"),
              QJsonObject{{QStringLiteral("x"), transform.position.x()},
                          {QStringLiteral("y"), transform.position.y()},
                          {QStringLiteral("angle"), transform.angle},
                          {QStringLiteral("sx"), transform.scale.x()},
                          {QStringLiteral("sy"), transform.scale.y()}});
}

std::optional<Channel> ChannelJson::ReadChannel(const QJsonObject& json,
                                                const QSet<Id>& layers,
                                                const QSet<Id>& points,
                                                JsonWarnings& warnings) {
  const std::optional<ChannelProperty> property =
      PropertyFrom(json.value(QStringLiteral("property")).toString());
  if (!property.has_value()) {
    warnings.Add(
        QStringLiteral("Dropped an animation channel of a kind "
                       "this app doesn't know"));
    return std::nullopt;
  }
  const Id target = json.value(QStringLiteral("target")).toInt(kNoId);
  const bool is_point = *property == ChannelProperty::kPointPosition;
  const bool has_target =
      is_point ? points.contains(target) : layers.contains(target);
  if (!has_target) {
    warnings.Add(
        QStringLiteral("Dropped the animation of %1 %2: it is "
                       "missing")
            .arg(is_point ? QStringLiteral("point") : QStringLiteral("layer"))
            .arg(target));
    return std::nullopt;
  }
  const bool is_single = *property == ChannelProperty::kLayerAngle;
  const double fallback = *property == ChannelProperty::kLayerScale ? 1.0 : 0.0;
  const double limit =
      *property == ChannelProperty::kLayerScale ? kMaxScale : kMaxCoordinate;
  Channel channel{.target = target, .property = *property, .keys = {}};
  for (const QJsonValue value : json.value(QStringLiteral("keys")).toArray()) {
    if (static_cast<int>(channel.keys.size()) >=
        ChannelManager::kMaxKeysPerChannel) {
      warnings.Add(QStringLiteral("Dropped keys past the limit"));
      break;
    }
    const QJsonObject json_key = value.toObject();
    Key key;
    key.frame = json_key.value(QStringLiteral("t")).toInt(0);
    if (key.frame < 1 || key.frame > kMaxFrame) {
      continue;
    }
    key.curve = Curves::FromFileName(
        json_key.value(QStringLiteral("curve")).toString());
    key.param =
        Finite(json_key.value(QStringLiteral("param")), 0, kMaxCoordinate);
    key.value =
        is_single
            ? QPointF(Finite(json_key.value(QStringLiteral("v")), 0,
                             kMaxCoordinate),
                      0)
            : QPointF(
                  Finite(json_key.value(QStringLiteral("x")), fallback, limit),
                  Finite(json_key.value(QStringLiteral("y")), fallback, limit));
    channel.keys.push_back(key);
  }
  if (channel.keys.empty()) {
    return std::nullopt;
  }
  return channel;
}

std::vector<Channel> ChannelJson::ReadChannels(const QJsonArray& list,
                                               const QSet<Id>& layers,
                                               const std::vector<Shape>& shapes,
                                               JsonWarnings& warnings) {
  QSet<Id> points;
  for (const Shape& shape : shapes) {
    for (const Point& point : shape.points) {
      points.insert(point.id);
    }
  }
  std::vector<Channel> channels;
  for (const QJsonValue value : list) {
    const bool is_full =
        static_cast<int>(channels.size()) >= ChannelManager::kMaxChannels;
    if (is_full) {
      warnings.Add(QStringLiteral("Dropped animation past the limit"));
      break;
    }
    std::optional<Channel> channel =
        ReadChannel(value.toObject(), layers, points, warnings);
    if (channel.has_value()) {
      channels.push_back(std::move(*channel));
    }
  }
  return channels;
}

void ChannelJson::ReadTransform(const QJsonObject& json, Layer& layer) {
  const QJsonArray origin = json.value(QStringLiteral("origin")).toArray();
  if (origin.size() == 2) {
    layer.origin = QPointF(Finite(origin[0], 0, kMaxCoordinate),
                           Finite(origin[1], 0, kMaxCoordinate));
  }
  const QJsonObject transform =
      json.value(QStringLiteral("transform")).toObject();
  LayerTransform& out = layer.transform;
  out.position =
      QPointF(Finite(transform.value(QStringLiteral("x")), 0, kMaxCoordinate),
              Finite(transform.value(QStringLiteral("y")), 0, kMaxCoordinate));
  out.angle =
      Finite(transform.value(QStringLiteral("angle")), 0, kMaxCoordinate);
  out.scale =
      QPointF(Finite(transform.value(QStringLiteral("sx")), 1, kMaxScale),
              Finite(transform.value(QStringLiteral("sy")), 1, kMaxScale));
}

}  // namespace snapper
