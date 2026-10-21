#pragma once

#include <QPointF>

#include <vector>

namespace snapper {

struct StrokeSample {
  QPointF pos;
  float pressure = 1.0F;
};

// A point of the finished line, before it gets an id.
struct StrokePoint {
  QPointF pos;
  float pressure = 1.0F;
  bool is_corner = false;
};

// Collects one pen stroke and smooths it as it comes in, so the line on
// screen is already the smooth one and nothing jumps when the pen lifts.
class PenStroke {
 public:
  static constexpr int kMaxSamples = 20000;

  // smoothing: 0 follows the pen exactly, 1 is the smoothest.
  // min_step: samples closer than this to the last one are skipped.
  PenStroke(double smoothing, double min_step);

  void Add(const QPointF& raw, float pressure);
  // Ends the stroke where the pen lifted, so the line reaches it.
  void Finish(const QPointF& raw, float pressure);

  const std::vector<StrokeSample>& Samples() const { return samples_; }
  bool IsEmpty() const { return samples_.empty(); }

  // The fewest points that stay within `tolerance` of the stroke and keep
  // its pressure changes. Sharp turns become corners.
  std::vector<StrokePoint> Simplify(double tolerance) const;

 private:
  double follow_;  // how far each smoothed sample moves toward the pen
  double min_step_;
  QPointF smoothed_;
  float smoothed_pressure_ = 1.0F;
  std::vector<StrokeSample> samples_;
};

}  // namespace snapper
