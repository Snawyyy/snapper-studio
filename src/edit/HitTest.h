#pragma once

#include <QPointF>
#include <QPolygonF>
#include <QSet>

#include "document/Ids.h"

namespace snapper {

class Document;
class ViewTransform;

// Finds what lies under the pointer. Distances are in screen pixels, so
// targets stay big enough for a pen at any zoom.
class HitTest {
 public:
  static constexpr double kPointRadiusPx = 8.0;
  static constexpr double kLineSlackPx = 5.0;

  // The topmost point on the layer within reach, or kNoId.
  static Id PointAt(const Document& document, Id layer,
                    const ViewTransform& view, const QPointF& screen);
  // The topmost shape on the layer whose fill or line is under the
  // pointer, or kNoId.
  static Id ShapeAt(const Document& document, Id layer,
                    const ViewTransform& view, const QPointF& screen);
  // The topmost visible layer with a shape under the pointer, or kNoId.
  static Id LayerAt(const Document& document, const ViewTransform& view,
                    const QPointF& screen);
  // Points on the layer that fall inside an area drawn on screen.
  static QSet<Id> PointsInside(const Document& document, Id layer,
                               const ViewTransform& view,
                               const QPolygonF& screen_area);
};

}  // namespace snapper
