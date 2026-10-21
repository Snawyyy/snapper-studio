#include "edit/tools/PenStroke.h"

#include <QLineF>

#include <algorithm>
#include <cmath>
#include <utility>

namespace snapper {

namespace {

// At full smoothing each sample still moves this far toward the pen.
constexpr double kMinFollow = 0.12;
// A pressure change this big is kept like a bend in the line.
constexpr double kPressureTolerance = 0.12;
// A turn sharper than this many degrees is a corner.
constexpr double kCornerDegrees = 70.0;

double DistanceToLine(const QPointF& p, const QPointF& a, const QPointF& b) {
  const QPointF ab = b - a;
  const double length_sq = QPointF::dotProduct(ab, ab);
  if (length_sq < 1e-12) {
    return QLineF(p, a).length();
  }
  const double t =
      std::clamp(QPointF::dotProduct(p - a, ab) / length_sq, 0.0, 1.0);
  return QLineF(p, a + ab * t).length();
}

double TurnDegrees(const QPointF& before, const QPointF& at,
                   const QPointF& after) {
  const QLineF in(before, at);
  const QLineF out(at, after);
  const double turn = std::abs(in.angleTo(out));
  return std::min(turn, 360.0 - turn);
}

}  // namespace

PenStroke::PenStroke(double smoothing, double min_step)
    : follow_(1.0 - std::clamp(smoothing, 0.0, 1.0) * (1.0 - kMinFollow)),
      min_step_(min_step) {}

void PenStroke::Add(const QPointF& raw, float pressure) {
  const bool is_full = static_cast<int>(samples_.size()) >= kMaxSamples;
  if (is_full) {
    return;
  }
  if (samples_.empty()) {
    smoothed_ = raw;
    smoothed_pressure_ = pressure;
    samples_.push_back({.pos = raw, .pressure = pressure});
    return;
  }
  smoothed_ += (raw - smoothed_) * follow_;
  smoothed_pressure_ += (pressure - smoothed_pressure_) * 0.5F;
  const bool is_far_enough =
      QLineF(samples_.back().pos, smoothed_).length() >= min_step_;
  if (is_far_enough) {
    samples_.push_back({.pos = smoothed_, .pressure = smoothed_pressure_});
  }
}

void PenStroke::Finish(const QPointF& raw, float pressure) {
  const bool has_room = static_cast<int>(samples_.size()) < kMaxSamples;
  const bool is_new_spot =
      samples_.empty() || QLineF(samples_.back().pos, raw).length() > 1e-6;
  if (has_room && is_new_spot) {
    samples_.push_back({.pos = raw, .pressure = pressure});
  }
}

// Ramer-Douglas-Peucker with a work list instead of recursion: a span is
// split at its farthest sample until every sample is close enough.
std::vector<StrokePoint> PenStroke::Simplify(double tolerance) const {
  const size_t count = samples_.size();
  if (count < 2) {
    return {};
  }
  std::vector<bool> keep(count, false);
  keep.front() = true;
  keep.back() = true;
  std::vector<std::pair<size_t, size_t>> spans = {{0, count - 1}};
  while (!spans.empty()) {
    const auto [first, last] = spans.back();
    spans.pop_back();
    const StrokeSample& a = samples_[first];
    const StrokeSample& b = samples_[last];
    double worst = 1.0;
    size_t worst_index = 0;
    for (size_t i = first + 1; i < last; ++i) {
      const double t =
          static_cast<double>(i - first) / static_cast<double>(last - first);
      const double expected_pressure =
          a.pressure + (b.pressure - a.pressure) * t;
      const double off_line =
          DistanceToLine(samples_[i].pos, a.pos, b.pos) / tolerance;
      const double off_pressure =
          std::abs(samples_[i].pressure - expected_pressure) /
          kPressureTolerance;
      const double off = std::max(off_line, off_pressure);
      if (off > worst) {
        worst = off;
        worst_index = i;
      }
    }
    if (worst_index != 0) {
      keep[worst_index] = true;
      spans.emplace_back(first, worst_index);
      spans.emplace_back(worst_index, last);
    }
  }

  std::vector<StrokePoint> points;
  for (size_t i = 0; i < count; ++i) {
    if (keep[i]) {
      points.push_back(
          {.pos = samples_[i].pos, .pressure = samples_[i].pressure});
    }
  }
  for (size_t i = 1; i + 1 < points.size(); ++i) {
    points[i].is_corner = TurnDegrees(points[i - 1].pos, points[i].pos,
                                      points[i + 1].pos) > kCornerDegrees;
  }
  return points;
}

}  // namespace snapper
