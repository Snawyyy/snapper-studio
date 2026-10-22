#include "document/ProjectJson.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include <algorithm>

#include "document/JsonWarnings.h"
#include "document/LookJson.h"
#include "document/StandardBrushes.h"

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
  QJsonObject json{
      {QStringLiteral("id"), shape.id},
      {QStringLiteral("layer"), shape.layer},
      {QStringLiteral("points"), points},
      {QStringLiteral("closed"), shape.closed},
      {QStringLiteral("fill"), LookJson::WriteColor(look.has_fill, look.fill)},
      {QStringLiteral("line"), LookJson::WriteColor(look.has_line, look.line)},
      {QStringLiteral("line_width"), look.line_width}};
  if (look.brush != kNoId) {
    json.insert(QStringLiteral("brush"), look.brush);
  }
  // Trailing empty slots are left out: [5] or [0, 5].
  const bool has_second = shape.styles[1] != kNoId;
  const bool has_first = shape.styles[0] != kNoId;
  if (has_second) {
    json.insert(QStringLiteral("styles"),
                QJsonArray{shape.styles[0], shape.styles[1]});
  } else if (has_first) {
    json.insert(QStringLiteral("styles"), QJsonArray{shape.styles[0]});
  }
  return json;
}

QJsonObject WritePoint(const Point& point) {
  return {{QStringLiteral("id"), point.id},
          {QStringLiteral("x"), point.pos.x()},
          {QStringLiteral("y"), point.pos.y()},
          {QStringLiteral("curve"), point.curve},
          {QStringLiteral("width"), point.width}};
}

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

// Version 1 had no brushes. Projects from it get the ones a new project
// starts with, under fresh ids.
void UpgradeFrom1(DocumentContents& contents) {
  for (const StandardBrush& standard : StandardBrushes()) {
    Brush brush = standard.brush;
    brush.id = contents.next_id++;
    brush.image = ImageStore::NameFor(standard.png);
    contents.images.insert(brush.image, standard.png);
    contents.brushes.push_back(brush);
  }
}

}  // namespace

QByteArray ProjectJson::Write(const DocumentContents& contents) {
  QJsonArray layers;
  for (const Layer& layer : contents.layers) {
    layers.append(WriteLayer(layer));
  }
  QJsonArray styles;
  for (const Style& style : contents.styles) {
    styles.append(LookJson::WriteStyle(style));
  }
  QJsonArray brushes;
  for (const Brush& brush : contents.brushes) {
    brushes.append(LookJson::WriteBrush(brush));
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
      {QStringLiteral("styles"), styles},
      {QStringLiteral("brushes"), brushes},
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

  JsonWarnings warnings;
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

  // Layers, brushes and styles share one id space.
  QSet<Id> taken = layer_ids;
  QSet<Id> brush_ids;
  for (const QJsonValue value :
       root.value(QStringLiteral("brushes")).toArray()) {
    const bool is_full =
        static_cast<int>(out.brushes.size()) >= BrushManager::kMaxBrushes;
    const std::optional<Brush> brush =
        is_full ? std::nullopt
                : LookJson::ReadBrush(value.toObject(), taken, warnings);
    if (brush.has_value()) {
      taken.insert(brush->id);
      brush_ids.insert(brush->id);
      highest = std::max(highest, brush->id);
      out.brushes.push_back(*brush);
    }
  }
  QSet<Id> style_ids;
  for (const QJsonValue value :
       root.value(QStringLiteral("styles")).toArray()) {
    const bool is_full =
        static_cast<int>(out.styles.size()) >= StyleManager::kMaxStyles;
    const std::optional<Style> style =
        is_full
            ? std::nullopt
            : LookJson::ReadStyle(value.toObject(), taken, brush_ids, warnings);
    if (style.has_value()) {
      taken.insert(style->id);
      style_ids.insert(style->id);
      highest = std::max(highest, style->id);
      out.styles.push_back(*style);
    }
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
                           taken.contains(shape.id);
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
    ShapeLook& look = shape.look;
    LookJson::ReadColor(json_shape.value(QStringLiteral("fill")), look.has_fill,
                        look.fill);
    LookJson::ReadColor(json_shape.value(QStringLiteral("line")), look.has_line,
                        look.line);
    look.brush = json_shape.value(QStringLiteral("brush")).toInt(kNoId);
    const QJsonArray styles =
        json_shape.value(QStringLiteral("styles")).toArray();
    for (int slot = 0; slot < kStylesPerShape && slot < styles.size(); ++slot) {
      shape.styles[static_cast<size_t>(slot)] = styles[slot].toInt(kNoId);
    }
    const bool has_lost_link =
        (look.brush != kNoId && !brush_ids.contains(look.brush)) ||
        std::ranges::any_of(shape.styles, [&](Id style) {
          return style != kNoId && !style_ids.contains(style);
        });
    if (has_lost_link) {
      warnings.Add(QStringLiteral("Shape %1 lost a style or brush that is "
                                  "missing")
                       .arg(shape.id));
      if (!brush_ids.contains(look.brush)) {
        look.brush = kNoId;
      }
      for (Id& style : shape.styles) {
        if (!style_ids.contains(style)) {
          style = kNoId;
        }
      }
    }
    shape.look.line_width = static_cast<float>(
        std::clamp(json_shape.value(QStringLiteral("line_width")).toDouble(3.0),
                   0.0, 1000.0));
    shape_ids.insert(shape.id);
    highest = std::max(highest, shape.id);
    out.shapes.push_back(std::move(shape));
  }

  out.next_id =
      std::max(root.value(QStringLiteral("next_id")).toInt(1), highest + 1);
  if (version < 2) {
    UpgradeFrom1(out);
  }
  loaded.warnings = warnings.Take();
  return loaded;
}

}  // namespace snapper
