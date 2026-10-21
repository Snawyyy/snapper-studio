#include "document/ProjectJson.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include <algorithm>

namespace snapper {

namespace {

constexpr int kMaxFps = 120;
constexpr int kMaxFrames = 100000;

QString LayerTypeName(LayerType type) {
  return type == LayerType::kImage ? QStringLiteral("image")
                                   : QStringLiteral("vector");
}

QJsonObject WriteCanvas(const CanvasSettings& canvas) {
  return {
      {QStringLiteral("width"), canvas.size.width()},
      {QStringLiteral("height"), canvas.size.height()},
      {QStringLiteral("fps"), canvas.fps},
      {QStringLiteral("frames"), canvas.frames},
      {QStringLiteral("background"), canvas.background.name(QColor::HexArgb)}};
}

QJsonObject WriteLayer(const Layer& layer) {
  QJsonObject json{{QStringLiteral("id"), layer.id},
                   {QStringLiteral("parent"), layer.parent},
                   {QStringLiteral("type"), LayerTypeName(layer.type)},
                   {QStringLiteral("name"), layer.name},
                   {QStringLiteral("visible"), layer.visible},
                   {QStringLiteral("locked"), layer.locked}};
  if (layer.type == LayerType::kImage) {
    json.insert(QStringLiteral("image"), layer.image);
    json.insert(QStringLiteral("x"), layer.image_pos.x());
    json.insert(QStringLiteral("y"), layer.image_pos.y());
    json.insert(QStringLiteral("scale"), layer.image_scale);
  }
  return json;
}

// A shape's look is written as colors, or null where it has no fill or no
// line.
QJsonObject WriteShape(const Shape& shape) {
  QJsonArray points;
  for (const Point& point : shape.points) {
    points.append(point.id);
  }
  const ShapeLook& look = shape.look;
  return {{QStringLiteral("id"), shape.id},
          {QStringLiteral("layer"), shape.layer},
          {QStringLiteral("points"), points},
          {QStringLiteral("closed"), shape.closed},
          {QStringLiteral("fill"),
           look.has_fill ? QJsonValue(look.fill.name(QColor::HexArgb))
                         : QJsonValue()},
          {QStringLiteral("line"),
           look.has_line ? QJsonValue(look.line.name(QColor::HexArgb))
                         : QJsonValue()},
          {QStringLiteral("line_width"), look.line_width}};
}

QJsonObject WritePoint(const Point& point) {
  return {{QStringLiteral("id"), point.id},
          {QStringLiteral("x"), point.pos.x()},
          {QStringLiteral("y"), point.pos.y()},
          {QStringLiteral("curve"), point.curve},
          {QStringLiteral("width"), point.width}};
}

// Collects warnings, keeping only the first few so a badly broken file
// can't flood the message.
class Warnings {
 public:
  void Add(const QString& text) {
    ++count_;
    if (list_.size() < ProjectJson::kMaxWarnings) {
      list_.append(text);
    }
  }
  QStringList Take() {
    const int hidden = count_ - static_cast<int>(list_.size());
    if (hidden > 0) {
      list_.append(QStringLiteral("and %1 more").arg(hidden));
    }
    return std::move(list_);
  }

 private:
  QStringList list_;
  int count_ = 0;
};

CanvasSettings ReadCanvas(const QJsonObject& json) {
  const CanvasSettings defaults;
  CanvasSettings canvas;
  canvas.size = QSize(
      std::clamp(
          json.value(QStringLiteral("width")).toInt(defaults.size.width()), 1,
          CanvasSettings::kMaxSide),
      std::clamp(
          json.value(QStringLiteral("height")).toInt(defaults.size.height()), 1,
          CanvasSettings::kMaxSide));
  canvas.fps = std::clamp(json.value(QStringLiteral("fps")).toInt(defaults.fps),
                          1, kMaxFps);
  canvas.frames =
      std::clamp(json.value(QStringLiteral("frames")).toInt(defaults.frames), 1,
                 kMaxFrames);
  const QColor background =
      QColor::fromString(json.value(QStringLiteral("background")).toString());
  canvas.background = background.isValid() ? background : defaults.background;
  return canvas;
}

// Reads a color, or nothing for null or a bad value.
std::optional<QColor> ReadColor(const QJsonValue& value) {
  const QColor color = QColor::fromString(value.toString());
  return color.isValid() ? std::optional<QColor>(color) : std::nullopt;
}

}  // namespace

QByteArray ProjectJson::Write(const DocumentContents& contents) {
  QJsonArray layers;
  for (const Layer& layer : contents.layers) {
    layers.append(WriteLayer(layer));
  }
  QJsonArray shapes;
  QJsonArray points;
  for (const Shape& shape : contents.shapes) {
    shapes.append(WriteShape(shape));
    for (const Point& point : shape.points) {
      points.append(WritePoint(point));
    }
  }
  const QJsonObject root{
      {QStringLiteral("format"), QStringLiteral("snapper")},
      {QStringLiteral("version"), kVersion},
      {QStringLiteral("app_version"), QStringLiteral(SNAPPER_VERSION)},
      {QStringLiteral("canvas"), WriteCanvas(contents.canvas)},
      {QStringLiteral("next_id"), contents.next_id},
      {QStringLiteral("layers"), layers},
      {QStringLiteral("shapes"), shapes},
      {QStringLiteral("points"), points},
  };
  return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

std::optional<LoadedProject> ProjectJson::Read(const QByteArray& json,
                                               QString* error) {
  QJsonParseError parse_error;
  const QJsonDocument doc = QJsonDocument::fromJson(json, &parse_error);
  const QJsonObject root = doc.object();
  const bool is_project =
      parse_error.error == QJsonParseError::NoError && doc.isObject() &&
      root.value(QStringLiteral("format")).toString() == u"snapper";
  if (!is_project) {
    *error = QStringLiteral("This file isn't a Snapper Studio project");
    return std::nullopt;
  }
  const int version = root.value(QStringLiteral("version")).toInt(0);
  if (version > kVersion) {
    *error = QStringLiteral(
                 "This project was saved by a newer Snapper Studio (format "
                 "%1). Update the app to open it.")
                 .arg(version);
    return std::nullopt;
  }
  if (version < 1) {
    *error = QStringLiteral("This project has no format version");
    return std::nullopt;
  }

  const QJsonArray point_list = root.value(QStringLiteral("points")).toArray();
  if (point_list.size() > ShapeManager::kMaxPointsPerProject) {
    *error = QStringLiteral("This project has more than %1 points")
                 .arg(ShapeManager::kMaxPointsPerProject);
    return std::nullopt;
  }

  Warnings warnings;
  LoadedProject loaded;
  DocumentContents& out = loaded.contents;
  out.canvas = ReadCanvas(root.value(QStringLiteral("canvas")).toObject());
  Id highest = 0;

  QSet<Id> layer_ids;
  for (const QJsonValue value :
       root.value(QStringLiteral("layers")).toArray()) {
    const QJsonObject json_layer = value.toObject();
    Layer layer;
    layer.id = json_layer.value(QStringLiteral("id")).toInt(kNoId);
    const bool is_bad_id = layer.id <= kNoId || layer_ids.contains(layer.id);
    const bool is_full =
        static_cast<int>(out.layers.size()) >= LayerManager::kMaxLayers;
    if (is_bad_id || is_full) {
      warnings.Add(QStringLiteral("Dropped a layer with a bad id"));
      continue;
    }
    layer.name = json_layer.value(QStringLiteral("name")).toString();
    layer.visible = json_layer.value(QStringLiteral("visible")).toBool(true);
    layer.locked = json_layer.value(QStringLiteral("locked")).toBool(false);
    const bool is_image =
        json_layer.value(QStringLiteral("type")).toString() == u"image";
    if (is_image) {
      layer.type = LayerType::kImage;
      layer.image = json_layer.value(QStringLiteral("image")).toString();
      layer.image_pos =
          QPointF(json_layer.value(QStringLiteral("x")).toDouble(),
                  json_layer.value(QStringLiteral("y")).toDouble());
      layer.image_scale = std::clamp(
          json_layer.value(QStringLiteral("scale")).toDouble(1.0), 1e-3, 1e3);
      if (!ImageStore::IsSafeName(layer.image)) {
        warnings.Add(QStringLiteral("Layer %1 names a picture that can't be "
                                    "read")
                         .arg(layer.name));
        layer.image.clear();
      }
    }
    // A parent must come earlier in the list.
    const Id parent = json_layer.value(QStringLiteral("parent")).toInt(kNoId);
    layer.parent = layer_ids.contains(parent) ? parent : kNoId;
    layer_ids.insert(layer.id);
    highest = std::max(highest, layer.id);
    out.layers.push_back(layer);
  }

  QHash<Id, Point> points;
  for (const QJsonValue value : point_list) {
    const QJsonObject json_point = value.toObject();
    Point point;
    point.id = json_point.value(QStringLiteral("id")).toInt(kNoId);
    if (point.id <= kNoId || points.contains(point.id)) {
      continue;
    }
    point.pos = QPointF(json_point.value(QStringLiteral("x")).toDouble(),
                        json_point.value(QStringLiteral("y")).toDouble());
    point.curve = static_cast<float>(std::clamp(
        json_point.value(QStringLiteral("curve")).toDouble(kSmoothCurve), 0.0,
        1.0));
    point.width = static_cast<float>(std::clamp(
        json_point.value(QStringLiteral("width")).toDouble(1.0), 0.0, 10.0));
    points.insert(point.id, point);
    highest = std::max(highest, point.id);
  }

  QSet<Id> shape_ids;
  QSet<Id> used_points;
  for (const QJsonValue value :
       root.value(QStringLiteral("shapes")).toArray()) {
    const QJsonObject json_shape = value.toObject();
    Shape shape;
    shape.id = json_shape.value(QStringLiteral("id")).toInt(kNoId);
    shape.layer = json_shape.value(QStringLiteral("layer")).toInt(kNoId);
    const bool is_bad_id = shape.id <= kNoId || shape_ids.contains(shape.id) ||
                           layer_ids.contains(shape.id);
    if (is_bad_id) {
      warnings.Add(QStringLiteral("Dropped a shape with a bad id"));
      continue;
    }
    if (!layer_ids.contains(shape.layer)) {
      warnings.Add(QStringLiteral("Dropped shape %1: its layer is missing")
                       .arg(shape.id));
      continue;
    }
    for (const QJsonValue point_id :
         json_shape.value(QStringLiteral("points")).toArray()) {
      const Id id = point_id.toInt(kNoId);
      const bool is_usable = points.contains(id) && !used_points.contains(id);
      if (is_usable) {
        shape.points.push_back(points.value(id));
        used_points.insert(id);
      }
    }
    if (shape.points.size() < 2) {
      warnings.Add(QStringLiteral("Dropped shape %1: its points are missing")
                       .arg(shape.id));
      continue;
    }
    shape.closed = json_shape.value(QStringLiteral("closed")).toBool(false) &&
                   shape.points.size() >= 3;
    const std::optional<QColor> fill =
        ReadColor(json_shape.value(QStringLiteral("fill")));
    const std::optional<QColor> line =
        ReadColor(json_shape.value(QStringLiteral("line")));
    shape.look.has_fill = fill.has_value();
    shape.look.fill = fill.value_or(shape.look.fill);
    shape.look.has_line = line.has_value();
    shape.look.line = line.value_or(shape.look.line);
    shape.look.line_width = static_cast<float>(
        std::clamp(json_shape.value(QStringLiteral("line_width")).toDouble(3.0),
                   0.0, 1000.0));
    shape_ids.insert(shape.id);
    highest = std::max(highest, shape.id);
    out.shapes.push_back(std::move(shape));
  }

  out.next_id =
      std::max(root.value(QStringLiteral("next_id")).toInt(1), highest + 1);
  loaded.warnings = warnings.Take();
  return loaded;
}

}  // namespace snapper
