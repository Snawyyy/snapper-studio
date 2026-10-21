#include "render/ShapeGeometry.h"

#include <QLineF>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <vector>

namespace snapper {

namespace {

constexpr int kMinSamplesPerSegment = 2;
constexpr int kMaxSamplesPerSegment = 64;
constexpr double kPixelsPerSample = 2.0;

struct Segment {
  QPointF from;
  QPointF control1;
  QPointF control2;
  QPointF to;
  float from_width = 1.0F;
  float to_width = 1.0F;
};

struct Sample {
  QPointF pos;
  double half_width = 0.0;
};

int SegmentCount(const Shape& shape) {
  const int points = static_cast<int>(shape.points.size());
  return shape.closed ? points : points - 1;
}

// The point at `index`, wrapping round on closed shapes and stopping at
// the ends on open ones.
const Point& PointAt(const Shape& shape, int index) {
  const int count = static_cast<int>(shape.points.size());
  const int wrapped = shape.closed ? (index % count + count) % count
                                   : std::clamp(index, 0, count - 1);
  return shape.points[static_cast<size_t>(wrapped)];
}

// A handle never reaches past this share of its own segment, so a short
// segment next to a long one can't overshoot into a hook.
constexpr double kMaxHandleShare = 0.5;

// The handle leaving `at` toward `toward`: along the line between its
// neighbors, as long as the curve value times half that line.
QPointF Handle(const Point& before, const Point& at, const Point& after,
               const QPointF& toward) {
  const QPointF along = after.pos - before.pos;
  const double span = std::hypot(along.x(), along.y());
  if (span < 1e-9) {
    return at.pos;
  }
  const double segment = QLineF(at.pos, toward).length();
  const double length =
      std::min(at.curve * span / 2, kMaxHandleShare * segment);
  const QPointF direction = along / span;
  const bool is_forward = QPointF::dotProduct(toward - at.pos, direction) >= 0;
  return at.pos + direction * (is_forward ? length : -length);
}

// A cubic piece from point i to i + 1. With every curve at kEllipseCurve,
// four points make an exact ellipse.
Segment SegmentAt(const Shape& shape, int i) {
  const Point& before = PointAt(shape, i - 1);
  const Point& from = PointAt(shape, i);
  const Point& to = PointAt(shape, i + 1);
  const Point& after = PointAt(shape, i + 2);
  return Segment{
      .from = from.pos,
      .control1 = Handle(before, from, to, to.pos),
      .control2 = Handle(from, to, after, from.pos),
      .to = to.pos,
      .from_width = from.width,
      .to_width = to.width,
  };
}

QPointF CubicAt(const Segment& s, double t) {
  const double u = 1 - t;
  return s.from * (u * u * u) + s.control1 * (3 * u * u * t) +
         s.control2 * (3 * u * t * t) + s.to * (t * t * t);
}

std::vector<Sample> SampleLine(const Shape& shape) {
  std::vector<Sample> samples;
  const double half_line = shape.look.line_width / 2.0;
  const int segments = SegmentCount(shape);
  for (int i = 0; i < segments; ++i) {
    const Segment s = SegmentAt(shape, i);
    const double reach = QLineF(s.from, s.control1).length() +
                         QLineF(s.control1, s.control2).length() +
                         QLineF(s.control2, s.to).length();
    const int steps = std::clamp(static_cast<int>(reach / kPixelsPerSample),
                                 kMinSamplesPerSegment, kMaxSamplesPerSegment);
    // Each segment skips its first sample, which the one before ended on.
    const int first = i == 0 ? 0 : 1;
    for (int step = first; step <= steps; ++step) {
      const double t = static_cast<double>(step) / steps;
      const double width = s.from_width + (s.to_width - s.from_width) * t;
      samples.push_back(
          {.pos = CubicAt(s, t), .half_width = half_line * width});
    }
  }
  return samples;
}

// Points left of the line at every sample, then right of it on the way
// back, so they go round the line once.
QPolygonF Ribbon(const std::vector<Sample>& samples) {
  const size_t count = samples.size();
  QPolygonF left;
  QPolygonF right;
  for (size_t i = 0; i < count; ++i) {
    const QPointF ahead = samples[std::min(i + 1, count - 1)].pos;
    const QPointF behind = samples[i == 0 ? 0 : i - 1].pos;
    const QPointF along = ahead - behind;
    const double length = std::hypot(along.x(), along.y());
    const bool has_direction = length > 1e-9;
    const QPointF normal =
        has_direction ? QPointF(-along.y(), along.x()) / length : QPointF();
    left << samples[i].pos + normal * samples[i].half_width;
    right.prepend(samples[i].pos - normal * samples[i].half_width);
  }
  left << right;
  return left;
}

// Twice the signed area: positive when the polygon runs clockwise on
// screen, where y points down.
double SignedArea(const QPolygonF& polygon) {
  double sum = 0.0;
  const qsizetype count = polygon.size();
  for (qsizetype i = 0; i < count; ++i) {
    const QPointF& a = polygon[i];
    const QPointF& b = polygon[(i + 1) % count];
    sum += a.x() * b.y() - b.x() * a.y();
  }
  return sum;
}

}  // namespace

QPainterPath ShapeGeometry::CenterLine(const Shape& shape) {
  QPainterPath path;
  if (shape.points.empty()) {
    return path;
  }
  path.moveTo(shape.points.front().pos);
  const int segments = SegmentCount(shape);
  for (int i = 0; i < segments; ++i) {
    const Segment s = SegmentAt(shape, i);
    path.cubicTo(s.control1, s.control2, s.to);
  }
  if (shape.closed) {
    path.closeSubpath();
  }
  return path;
}

QPainterPath ShapeGeometry::LineOutline(const Shape& shape) {
  QPainterPath path;
  path.setFillRule(Qt::WindingFill);
  const std::vector<Sample> samples = SampleLine(shape);
  if (samples.empty()) {
    return path;
  }
  // Circles run clockwise. The ribbon must too, or where they overlap the
  // winding cancels out and leaves holes.
  QPolygonF ribbon = Ribbon(samples);
  if (SignedArea(ribbon) < 0) {
    std::ranges::reverse(ribbon);
  }
  path.addPolygon(ribbon);
  // Round joins and ends: a circle at every point the shape is built on.
  const double half_line = shape.look.line_width / 2.0;
  for (const Point& point : shape.points) {
    const double radius = half_line * point.width;
    path.addEllipse(point.pos, radius, radius);
  }
  return path;
}

bool ShapeGeometry::HasEvenWidth(const Shape& shape) {
  const float first = shape.points.empty() ? 1.0F : shape.points[0].width;
  return std::ranges::all_of(shape.points, [first](const Point& point) {
    return point.width == first;
  });
}

}  // namespace snapper
