#include "edit/HitTest.h"

#include <QLineF>
#include <QPainterPathStroker>

#include "document/Document.h"
#include "render/ShapeGeometry.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

bool IsShapeUnder(const Shape& shape, const QPointF& canvas,
                  double canvas_per_screen) {
  const QPainterPath line = ShapeGeometry::CenterLine(shape);
  const bool is_in_fill =
      shape.closed && shape.look.has_fill && line.contains(canvas);
  if (is_in_fill) {
    return true;
  }
  QPainterPathStroker stroker;
  stroker.setWidth(shape.look.line_width +
                   2 * HitTest::kLineSlackPx * canvas_per_screen);
  return stroker.createStroke(line).contains(canvas);
}

}  // namespace

Id HitTest::PointAt(const Document& document, Id layer,
                    const ViewTransform& view, const QPointF& screen) {
  const std::vector<Shape>& shapes = document.Shapes().All();
  Id best = kNoId;
  double best_distance = kPointRadiusPx;
  // Top to bottom, and only a closer point beats one found higher up.
  for (auto it = shapes.rbegin(); it != shapes.rend(); ++it) {
    if (it->layer != layer) {
      continue;
    }
    for (const Point& point : it->points) {
      const double distance = QLineF(view.ToScreen(point.pos), screen).length();
      if (distance < best_distance) {
        best_distance = distance;
        best = point.id;
      }
    }
  }
  return best;
}

Id HitTest::ShapeAt(const Document& document, Id layer,
                    const ViewTransform& view, const QPointF& screen) {
  const QPointF canvas = view.ToCanvas(screen);
  const double canvas_per_screen = 1.0 / view.Zoom();
  const std::vector<Shape>& shapes = document.Shapes().All();
  for (auto it = shapes.rbegin(); it != shapes.rend(); ++it) {
    const bool is_hit =
        it->layer == layer && IsShapeUnder(*it, canvas, canvas_per_screen);
    if (is_hit) {
      return it->id;
    }
  }
  return kNoId;
}

Id HitTest::LayerAt(const Document& document, const ViewTransform& view,
                    const QPointF& screen) {
  const std::vector<Layer>& layers = document.Layers().All();
  for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
    if (!it->visible) {
      continue;
    }
    if (ShapeAt(document, it->id, view, screen) != kNoId) {
      return it->id;
    }
  }
  return kNoId;
}

QSet<Id> HitTest::PointsInside(const Document& document, Id layer,
                               const ViewTransform& view,
                               const QPolygonF& screen_area) {
  QSet<Id> inside;
  for (const Shape& shape : document.Shapes().All()) {
    if (shape.layer != layer) {
      continue;
    }
    for (const Point& point : shape.points) {
      const bool is_inside =
          screen_area.containsPoint(view.ToScreen(point.pos), Qt::OddEvenFill);
      if (is_inside) {
        inside.insert(point.id);
      }
    }
  }
  return inside;
}

}  // namespace snapper
