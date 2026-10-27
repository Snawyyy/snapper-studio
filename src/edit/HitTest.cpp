#include "edit/HitTest.h"

#include <QLineF>
#include <QPainterPathStroker>

#include <algorithm>
#include <cmath>

#include "document/Document.h"
#include "eval/Pose.h"
#include "render/ShapeGeometry.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

// Where the pointer is in the layer's own space, and how many of its units
// make one screen pixel.
struct LayerSpot {
  QPointF pos;
  double units_per_screen = 1.0;
};

LayerSpot SpotOn(const Pose& pose, Id layer, const ViewTransform& view,
                 const QPointF& screen) {
  const QTransform matrix = pose.LayerMatrix(layer);
  const double scale = std::sqrt(std::abs(matrix.determinant()));
  const bool is_flat = !matrix.isInvertible() || scale <= 0;
  if (is_flat) {
    return {.pos = view.ToCanvas(screen),
            .units_per_screen = 1.0 / view.Zoom()};
  }
  return {.pos = matrix.inverted().map(view.ToCanvas(screen)),
          .units_per_screen = 1.0 / (view.Zoom() * scale)};
}

bool IsOnLineAt(const Document& document, const Shape& shape,
                const LayerSpot& spot) {
  const ShapeLook look = document.Styles().Resolve(shape.look, shape.styles);
  QPainterPathStroker stroker;
  stroker.setWidth(look.line_width +
                   2 * HitTest::kLineSlackPx * spot.units_per_screen);
  return stroker.createStroke(ShapeGeometry::CenterLine(shape))
      .contains(spot.pos);
}

bool IsShapeUnder(const Document& document, const Shape& shape,
                  const LayerSpot& spot) {
  const ShapeLook look = document.Styles().Resolve(shape.look, shape.styles);
  const bool is_in_fill = shape.closed && look.has_fill &&
                          ShapeGeometry::CenterLine(shape).contains(spot.pos);
  return is_in_fill || IsOnLineAt(document, shape, spot);
}

}  // namespace

Id HitTest::PointAt(const Pose& pose, Id layer, const ViewTransform& view,
                    const QPointF& screen) {
  const QTransform to_screen = pose.LayerMatrix(layer) * view.ToScreenMatrix();
  Id best = kNoId;
  double best_distance = kPointRadiusPx;
  // Top to bottom, and only a closer point beats one found higher up.
  for (auto it = pose.shapes.rbegin(); it != pose.shapes.rend(); ++it) {
    if (it->layer != layer) {
      continue;
    }
    for (const Point& point : it->points) {
      const double distance = QLineF(to_screen.map(point.pos), screen).length();
      if (distance < best_distance) {
        best_distance = distance;
        best = point.id;
      }
    }
  }
  return best;
}

Id HitTest::ShapeAt(const Document& document, const Pose& pose, Id layer,
                    const ViewTransform& view, const QPointF& screen) {
  const LayerSpot spot = SpotOn(pose, layer, view, screen);
  for (auto it = pose.shapes.rbegin(); it != pose.shapes.rend(); ++it) {
    const bool is_hit = it->layer == layer && IsShapeUnder(document, *it, spot);
    if (is_hit) {
      return it->id;
    }
  }
  return kNoId;
}

bool HitTest::IsOnLine(const Document& document, const Pose& pose,
                       const Shape& shape, const ViewTransform& view,
                       const QPointF& screen) {
  return IsOnLineAt(document, shape, SpotOn(pose, shape.layer, view, screen));
}

Id HitTest::LayerAt(const Document& document, const Pose& pose,
                    const ViewTransform& view, const QPointF& screen) {
  const std::vector<Layer>& layers = document.Layers().All();
  for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
    if (!document.Layers().IsShown(it->id)) {
      continue;
    }
    if (ShapeAt(document, pose, it->id, view, screen) != kNoId) {
      return it->id;
    }
  }
  return kNoId;
}

BoneHit HitTest::BoneAt(const Pose& pose, Id bone_layer,
                        const ViewTransform& view, const QPointF& screen) {
  const QTransform to_screen =
      pose.LayerMatrix(bone_layer) * view.ToScreenMatrix();
  BoneHit best;
  double best_distance = kBoneReachPx;
  for (auto it = pose.bones.cbegin(); it != pose.bones.cend(); ++it) {
    if (it->layer != bone_layer) {
      continue;
    }
    const QPointF base = to_screen.map(it->Base());
    const QPointF tip = to_screen.map(it->Tip());
    const QPointF span = tip - base;
    const double squared = QPointF::dotProduct(span, span);
    const double along =
        squared <= 0
            ? 0.0
            : std::clamp(QPointF::dotProduct(screen - base, span) / squared,
                         0.0, 1.0);
    const double distance = QLineF(base + span * along, screen).length();
    // Ties go to the bone added later, which is drawn on top.
    const bool is_nearer = distance < best_distance ||
                           (distance == best_distance && best.bone != kNoId &&
                            it.key() > best.bone);
    if (!is_nearer) {
      continue;
    }
    best_distance = distance;
    const bool is_tip =
        along > 0.7 || QLineF(tip, screen).length() <= kPointRadiusPx;
    const bool is_base = !is_tip && along < 0.15;
    best = {.bone = it.key(),
            .part = is_tip    ? BonePart::kTip
                    : is_base ? BonePart::kBase
                              : BonePart::kBody,
            .along = along};
  }
  return best;
}

QSet<Id> HitTest::PointsInside(const Pose& pose, Id layer,
                               const ViewTransform& view,
                               const QPolygonF& screen_area) {
  const QTransform to_screen = pose.LayerMatrix(layer) * view.ToScreenMatrix();
  QSet<Id> inside;
  for (const Shape& shape : pose.shapes) {
    if (shape.layer != layer) {
      continue;
    }
    for (const Point& point : shape.points) {
      const bool is_inside =
          screen_area.containsPoint(to_screen.map(point.pos), Qt::OddEvenFill);
      if (is_inside) {
        inside.insert(point.id);
      }
    }
  }
  return inside;
}

}  // namespace snapper
