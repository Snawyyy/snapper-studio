#include "eval/IkSolver.h"

#include <QLineF>

#include <cmath>
#include <numbers>

namespace snapper {

namespace {

QPointF TurnedAround(const QPointF& point, const QPointF& pivot, double angle) {
  const QPointF offset = point - pivot;
  const double cos = std::cos(angle);
  const double sin = std::sin(angle);
  return pivot + QPointF(offset.x() * cos - offset.y() * sin,
                         offset.x() * sin + offset.y() * cos);
}

// Into -pi to pi, so a bone never spins the long way round.
double Wrapped(double angle) {
  return std::remainder(angle, 2 * std::numbers::pi);
}

}  // namespace

std::vector<double> IkSolver::Solve(std::vector<QPointF> bases, QPointF tip,
                                    const QPointF& target) {
  std::vector<double> turns(bases.size(), 0.0);
  for (int round = 0; round < kRounds; ++round) {
    if (QLineF(tip, target).length() <= kReach) {
      break;
    }
    for (size_t i = bases.size(); i-- > 0;) {
      const QPointF to_tip = tip - bases[i];
      const QPointF to_target = target - bases[i];
      const bool is_degenerate =
          to_tip.manhattanLength() < 1e-9 || to_target.manhattanLength() < 1e-9;
      if (is_degenerate) {
        continue;
      }
      const double turn = Wrapped(std::atan2(to_target.y(), to_target.x()) -
                                  std::atan2(to_tip.y(), to_tip.x()));
      turns[i] += turn;
      // Everything past this bone turns with it.
      for (size_t j = i + 1; j < bases.size(); ++j) {
        bases[j] = TurnedAround(bases[j], bases[i], turn);
      }
      tip = TurnedAround(tip, bases[i], turn);
    }
  }
  for (double& turn : turns) {
    turn = Wrapped(turn);
  }
  return turns;
}

}  // namespace snapper
