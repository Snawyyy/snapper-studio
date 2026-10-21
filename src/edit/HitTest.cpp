#include "edit/HitTest.h"

#include <QLineF>
#include <QPainterPathStroker>

#include "document/Document.h"
#include "render/ShapeGeometry.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

bool IsOnLineAt(const Document& document, const Shape& shape,
                const QPointF& canvas, double canvas_per_screen) {
  const ShapeLook look = document.Styles().Resolve(shape.look, shape.styles);
  QPainterPathStroker stroker;
  stroker.setWidth(look.line_width +
                   2 * HitTest::kLineSlackPx * canvas_per_screen);
  return stroker.createStroke(ShapeGeometry::CenterLine(shape))
      .contains(canvas);
}

bool IsShapeUnder(const Document& document, const Shape& shape,
                  const QPointF& canvas, double canvas_per_screen) {
  const ShapeLook look = document.Styles().Resolve(shape.look, shape.styles);
  const bool is_in_fill = shape.closed && look.has_fill &&
                          ShapeGeometry::CenterLine(shape).contains(canvas);
  return is_in_fill || IsOnLineAt(document, shape, canvas, canvas_per_screen);
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
    const bool is_hit = it->layer == layer &&
                        IsShapeUnder(document, *it, canvas, canvas_per_screen);
    if (is_hit) {
      return it->id;
    }
  }
  return kNoId;
}

bool HitTest::IsOnLine(const Document& document, const Shape& shape,
                       const ViewTransform& view, const QPointF& screen) {
  return IsOnLineAt(document, shape, view.ToCanvas(screen), 1.0 / view.Zoom());
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
