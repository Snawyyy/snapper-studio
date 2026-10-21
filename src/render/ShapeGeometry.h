#pragma once

#include <QPainterPath>

#include <vector>

#include "document/ShapeManager.h"

namespace snapper {

// A spot along a line, and the share of the full line width there.
struct LineSample {
  QPointF pos;
  double width = 1.0;
};

// Turns a shape's points into drawable paths. Each point's curve value
// sets how smoothly the line passes through it, from a sharp corner (0) to
// a natural curve through its neighbors.
class ShapeGeometry {
 public:
  // The center line, for fills, hit tests and plain lines.
  static QPainterPath CenterLine(const Shape& shape);

  // The filled outline of a line `line_width` wide, whose width follows
  // each point's width (pen pressure) along the way, with round ends and
  // joins.
  static QPainterPath LineOutline(const Shape& shape, double line_width);

  // Spots along the center line, about two pixels apart, from the first
  // point to the last (back to the first on closed shapes).
  static std::vector<LineSample> Walk(const Shape& shape);

  // True when every point has the same width, so a plain pen can draw it.
  static bool HasEvenWidth(const Shape& shape);
};

}  // namespace snapper
