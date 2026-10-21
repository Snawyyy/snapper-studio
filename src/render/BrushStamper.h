#pragma once

#include <QImage>
#include <QPointF>

#include <vector>

#include "document/BrushManager.h"
#include "document/ShapeManager.h"

class QPainter;

namespace snapper {

// One print of a brush picture.
struct BrushStamp {
  QPointF pos;
  // Across the picture's width, in canvas pixels.
  double size = 0.0;
  double angle_degrees = 0.0;
};

// Stamps a brush's picture along a line with its spacing, turn and
// jitter. The jitter is random but seeded by the shape, so a line looks
// the same every time it is drawn, until a boil frame picks a new seed.
class BrushStamper {
 public:
  static constexpr int kMaxStampsPerShape = 20000;
  // Stamps never sit closer than this, in canvas pixels.
  static constexpr double kMinGap = 0.25;

  // Where every stamp goes, for a line `line_width` wide.
  static std::vector<BrushStamp> Plan(const Shape& shape, double line_width,
                                      const Brush& brush, int frame);

  // Stamps the picture, tinted with `color`, along the line. The painter
  // is set to canvas coordinates.
  static void Draw(QPainter& painter, const Shape& shape, double line_width,
                   const QColor& color, const Brush& brush,
                   const QImage& picture, int frame);

  // The picture as paint coverage in `color`, `side` pixels across. A
  // picture without transparency counts its dark parts as paint.
  static QImage Tint(const QImage& picture, const QColor& color, int side);
};

}  // namespace snapper
