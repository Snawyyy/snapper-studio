#pragma once

#include <QColor>
#include <QHash>
#include <QObject>
#include <QPointF>

#include <optional>
#include <vector>

#include "document/Ids.h"

namespace snapper {

// How smooth the line is at a point: 0 is a sharp corner, kSmoothCurve is
// a natural curve through its neighbors.
constexpr float kSmoothCurve = 0.33F;
// Four points with this curve, at the ends of an ellipse's axes, draw the
// ellipse.
constexpr float kEllipseCurve = 0.5523F;

struct Point {
  Id id = kNoId;
  QPointF pos{};
  float curve = kSmoothCurve;
  // Scales the shape's line width here, from pen pressure. 1 is full width.
  float width = 1.0F;
};

// A shape's own fill and line. Styles shared between shapes come in M2.
struct ShapeLook {
  bool has_fill = false;
  QColor fill = Qt::white;
  bool has_line = true;
  QColor line = Qt::black;
  float line_width = 3.0F;
};

struct Shape {
  Id id = kNoId;
  Id layer = kNoId;
  std::vector<Point> points;
  bool closed = false;
  ShapeLook look{};
};

struct RemovedShape {
  Shape shape{};
  int index = 0;
};

struct PointPosition {
  Id point = kNoId;
  QPointF pos{};
};

// Owns the contents of vector layers: shapes and their points. Shapes sit
// in one flat list in drawing order, each one naming its layer.
class ShapeManager : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMaxShapes = 100000;
  static constexpr int kMaxPointsPerShape = 10000;
  static constexpr int kMaxPointsPerProject = 1000000;

  explicit ShapeManager(QObject* parent = nullptr);

  // Inserts at index (clamped), so later shapes draw on top. Fails on a
  // missing or repeated id, too few or too many points, or a full project.
  bool Insert(const Shape& shape, int index);
  std::optional<RemovedShape> Take(Id id);
  void Clear();

  // Moves points. Unknown ids are skipped. Returns how many moved.
  int SetPositions(const std::vector<PointPosition>& positions);

  const Shape* Find(Id id) const;
  int IndexOf(Id id) const;
  // kNoId when no shape holds the point.
  Id ShapeOfPoint(Id point) const;
  std::optional<QPointF> PositionOf(Id point) const;

  int Count() const { return static_cast<int>(shapes_.size()); }
  int PointCount() const { return static_cast<int>(point_index_.size()); }
  const std::vector<Shape>& All() const { return shapes_; }
  std::vector<Id> ShapesOn(Id layer) const;

 Q_SIGNALS:
  // Something on this layer changed: shapes added, removed, or moved.
  void LayerContentChanged(Id layer);

 private:
  struct PointRef {
    int shape = 0;
    int point = 0;
  };

  void RebuildIndex();

  std::vector<Shape> shapes_;
  QHash<Id, int> shape_index_;
  QHash<Id, PointRef> point_index_;
};

}  // namespace snapper
