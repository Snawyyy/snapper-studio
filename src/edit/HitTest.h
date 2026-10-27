#pragma once

#include <QPointF>
#include <QPolygonF>
#include <QSet>

#include <cstdint>

#include "document/Ids.h"

namespace snapper {

class Document;
struct Pose;
struct Shape;
class ViewTransform;

enum class BonePart : std::uint8_t { kBase, kBody, kTip };

struct BoneHit {
  Id bone = kNoId;
  BonePart part = BonePart::kBody;
  // Where along it the pointer is, 0 at the base and 1 at the tip.
  double along = 0.0;
};

// Finds what lies under the pointer, as posed on the current frame.
// Distances are in screen pixels, so targets stay big enough for a pen at
// any zoom.
class HitTest {
 public:
  static constexpr double kPointRadiusPx = 8.0;
  static constexpr double kLineSlackPx = 5.0;
  static constexpr double kBoneReachPx = 9.0;

  // The topmost point on the layer within reach, or kNoId.
  static Id PointAt(const Pose& pose, Id layer, const ViewTransform& view,
                    const QPointF& screen);
  // The topmost shape on the layer whose fill or line is under the
  // pointer, or kNoId.
  static Id ShapeAt(const Document& document, const Pose& pose, Id layer,
                    const ViewTransform& view, const QPointF& screen);
  // True when the pointer is on the shape's line, rather than its fill.
  static bool IsOnLine(const Document& document, const Pose& pose,
                       const Shape& shape, const ViewTransform& view,
                       const QPointF& screen);
  // The topmost visible layer with a shape under the pointer, or kNoId.
  static Id LayerAt(const Document& document, const Pose& pose,
                    const ViewTransform& view, const QPointF& screen);
  // The bone of the bone layer nearest the pointer and within reach.
  // kNoId in the hit for none.
  static BoneHit BoneAt(const Pose& pose, Id bone_layer,
                        const ViewTransform& view, const QPointF& screen);
  // Points on the layer that fall inside an area drawn on screen.
  static QSet<Id> PointsInside(const Pose& pose, Id layer,
                               const ViewTransform& view,
                               const QPolygonF& screen_area);
};

}  // namespace snapper
