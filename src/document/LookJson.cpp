#include "document/LookJson.h"

#include <algorithm>

#include "document/ImageStore.h"

namespace snapper {

namespace {

constexpr double kMaxLineWidth = 1000.0;

float ReadFloat(const QJsonObject& json, const char* key, float fallback) {
  return static_cast<float>(
      json.value(QLatin1StringView(key)).toDouble(fallback));
}

}  // namespace

QJsonValue LookJson::WriteColor(bool is_on, const QColor& color) {
  return is_on ? QJsonValue(color.name(QColor::HexArgb)) : QJsonValue();
}

void LookJson::ReadColor(const QJsonValue& value, bool& is_on, QColor& color) {
  if (value.isNull()) {
    is_on = false;
    return;
  }
  const QColor read = QColor::fromString(value.toString());
  if (read.isValid()) {
    is_on = true;
    color = read;
  }
}

QJsonObject LookJson::WriteStyle(const Style& style) {
  QJsonObject json{{QStringLiteral("id"), style.id},
                   {QStringLiteral("name"), style.name}};
  const ShapeLook& look = style.look;
  if (style.parts.fill) {
    json.insert(QStringLiteral("fill"), WriteColor(look.has_fill, look.fill));
  }
  if (style.parts.line) {
    json.insert(QStringLiteral("line"), WriteColor(look.has_line, look.line));
  }
  if (style.parts.width) {
    json.insert(QStringLiteral("width"), look.line_width);
  }
  if (style.parts.brush) {
    json.insert(QStringLiteral("brush"), look.brush);
  }
  return json;
}

QJsonObject LookJson::WriteBrush(const Brush& brush) {
  return {{QStringLiteral("id"), brush.id},
          {QStringLiteral("name"), brush.name},
          {QStringLiteral("image"), brush.image},
          {QStringLiteral("spacing"), brush.spacing},
          {QStringLiteral("align"), brush.align},
          {QStringLiteral("angle_jitter"), brush.angle_jitter},
          {QStringLiteral("spacing_jitter"), brush.spacing_jitter},
          {QStringLiteral("size_jitter"), brush.size_jitter},
          {QStringLiteral("boil_every"), brush.boil_every}};
}

std::optional<Brush> LookJson::ReadBrush(const QJsonObject& json,
                                         const QSet<Id>& taken,
                                         JsonWarnings& warnings) {
  Brush brush;
  brush.id = json.value(QStringLiteral("id")).toInt(kNoId);
  const bool is_bad_id = brush.id <= kNoId || taken.contains(brush.id);
  if (is_bad_id) {
    warnings.Add(QStringLiteral("Dropped a brush with a bad id"));
    return std::nullopt;
  }
  brush.name = json.value(QStringLiteral("name")).toString();
  brush.image = json.value(QStringLiteral("image")).toString();
  if (!ImageStore::IsSafeName(brush.image)) {
    warnings.Add(QStringLiteral("Dropped brush %1: its picture can't be read")
                     .arg(brush.name));
    return std::nullopt;
  }
  const Brush defaults;
  brush.spacing = ReadFloat(json, "spacing", defaults.spacing);
  brush.align = json.value(QStringLiteral("align")).toBool(defaults.align);
  brush.angle_jitter = ReadFloat(json, "angle_jitter", defaults.angle_jitter);
  brush.spacing_jitter =
      ReadFloat(json, "spacing_jitter", defaults.spacing_jitter);
  brush.size_jitter = ReadFloat(json, "size_jitter", defaults.size_jitter);
  brush.boil_every =
      json.value(QStringLiteral("boil_every")).toInt(defaults.boil_every);
  return brush.Clamped();
}

std::optional<Style> LookJson::ReadStyle(const QJsonObject& json,
                                         const QSet<Id>& taken,
                                         const QSet<Id>& brushes,
                                         JsonWarnings& warnings) {
  Style style;
  style.id = json.value(QStringLiteral("id")).toInt(kNoId);
  const bool is_bad_id = style.id <= kNoId || taken.contains(style.id);
  if (is_bad_id) {
    warnings.Add(QStringLiteral("Dropped a style with a bad id"));
    return std::nullopt;
  }
  style.name = json.value(QStringLiteral("name")).toString();
  ShapeLook& look = style.look;
  StyleParts& parts = style.parts;
  parts.fill = json.contains(QStringLiteral("fill"));
  if (parts.fill) {
    ReadColor(json.value(QStringLiteral("fill")), look.has_fill, look.fill);
  }
  parts.line = json.contains(QStringLiteral("line"));
  if (parts.line) {
    ReadColor(json.value(QStringLiteral("line")), look.has_line, look.line);
  }
  parts.width = json.contains(QStringLiteral("width"));
  if (parts.width) {
    look.line_width = static_cast<float>(std::clamp(
        json.value(QStringLiteral("width")).toDouble(3.0), 0.0, kMaxLineWidth));
  }
  parts.brush = json.contains(QStringLiteral("brush"));
  if (parts.brush) {
    look.brush = json.value(QStringLiteral("brush")).toInt(kNoId);
    const bool is_missing =
        look.brush != kNoId && !brushes.contains(look.brush);
    if (is_missing) {
      warnings.Add(QStringLiteral("Style %1 lost its brush, which is missing")
                       .arg(style.name));
      look.brush = kNoId;
    }
  }
  return style;
}

}  // namespace snapper
