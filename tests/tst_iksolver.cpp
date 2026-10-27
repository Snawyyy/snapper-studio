#include <QLineF>
#include <QTest>

#include <cmath>
#include <numbers>

#include "eval/IkSolver.h"

using snapper::IkSolver;

namespace {

// Where the tip ends up after turning each bone of a straight chain.
QPointF TipAfter(const std::vector<QPointF>& bases, const QPointF& tip,
                 const std::vector<double>& turns) {
  std::vector<QPointF> points = bases;
  points.push_back(tip);
  double heading = 0;
  QPointF at = points[0];
  for (size_t i = 0; i < bases.size(); ++i) {
    heading += turns[i];
    const QPointF span = points[i + 1] - points[i];
    const double length = std::hypot(span.x(), span.y());
    const double rest = std::atan2(span.y(), span.x());
    at += QPointF(length * std::cos(rest + heading),
                  length * std::sin(rest + heading));
  }
  return at;
}

}  // namespace

class IkSolverTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void OneBonePointsAtTheTarget() {
    const std::vector<double> turns =
        IkSolver::Solve({QPointF(0, 0)}, QPointF(10, 0), QPointF(0, 50));
    QCOMPARE(turns.size(), size_t{1});
    QVERIFY(std::abs(turns[0] - std::numbers::pi / 2) < 1e-9);
  }

  void ATwoBoneArmReachesASpotInReach() {
    const std::vector<QPointF> bases = {QPointF(0, 0), QPointF(10, 0)};
    const QPointF tip(20, 0);
    const QPointF target(5, 12);
    const std::vector<double> turns = IkSolver::Solve(bases, tip, target);
    QVERIFY(QLineF(TipAfter(bases, tip, turns), target).length() < 0.05);
  }

  void AnOutOfReachSpotStraightensTheChainTowardsIt() {
    const std::vector<QPointF> bases = {QPointF(0, 0), QPointF(10, 0)};
    const std::vector<double> turns =
        IkSolver::Solve(bases, QPointF(20, 0), QPointF(0, -100));
    const QPointF reached = TipAfter(bases, QPointF(20, 0), turns);
    QVERIFY(QLineF(reached, QPointF(0, -20)).length() < 0.5);
  }
};

QTEST_GUILESS_MAIN(IkSolverTest)
#include "tst_iksolver.moc"
