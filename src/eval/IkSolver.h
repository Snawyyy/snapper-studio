#pragma once

#include <QPointF>

#include <vector>

namespace snapper {

// Turns a chain of bones so the tip of the last one reaches a spot, one
// bone at a time from the tip back, over a few rounds (cyclic coordinate
// descent). Works in one space, the bone layer's.
class IkSolver {
 public:
  static constexpr int kRounds = 24;
  // Close enough, in canvas pixels.
  static constexpr double kReach = 0.01;

  // `bases` holds each bone's base, root first. Returns how far to turn
  // each bone, in radians, in the same order.
  static std::vector<double> Solve(std::vector<QPointF> bases, QPointF tip,
                                   const QPointF& target);
};

}  // namespace snapper
