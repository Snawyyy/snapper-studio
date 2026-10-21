#include <QLineF>
#include <QTest>

#include <cmath>
#include <numbers>

#include "edit/tools/PenStroke.h"

using snapper::PenStroke;
using snapper::StrokePoint;

class PenStrokeTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void StraightLineNeedsTwoPoints() {
    PenStroke stroke(0.0, 0.5);
    for (int x = 0; x <= 100; ++x) {
      stroke.Add(QPointF(x, 0), 1.0F);
    }
    stroke.Finish(QPointF(100, 0), 1.0F);
    const std::vector<StrokePoint> points = stroke.Simplify(1.0);
    QCOMPARE(points.size(), size_t{2});
  }

  void SharpTurnBecomesACorner() {
    PenStroke stroke(0.0, 0.5);
    for (int x = 0; x <= 50; ++x) {
      stroke.Add(QPointF(x, 0), 1.0F);
    }
    for (int y = 1; y <= 50; ++y) {
      stroke.Add(QPointF(50, y), 1.0F);
    }
    const std::vector<StrokePoint> points = stroke.Simplify(1.0);
    QCOMPARE(points.size(), size_t{3});
    QVERIFY(points[1].is_corner);
    QVERIFY(QLineF(points[1].pos, QPointF(50, 0)).length() < 1.0);
  }

  void PressureChangesAreKept() {
    PenStroke stroke(0.0, 0.5);
    for (int x = 0; x <= 100; ++x) {
      const float pressure = x <= 50 ? 0.2F : 1.0F;
      stroke.Add(QPointF(x, 0), pressure);
    }
    QVERIFY(stroke.Simplify(1.0).size() > 2);
  }

  void SmoothingCalmsAShakyHand() {
    PenStroke raw(0.0, 0.0);
    PenStroke smooth(1.0, 0.0);
    for (int x = 0; x <= 200; ++x) {
      const QPointF shaky(x, (x % 2 == 0) ? 3.0 : -3.0);
      raw.Add(shaky, 1.0F);
      smooth.Add(shaky, 1.0F);
    }
    double raw_spread = 0.0;
    double smooth_spread = 0.0;
    for (size_t i = 100; i < 200; ++i) {
      raw_spread = std::max(raw_spread, std::abs(raw.Samples()[i].pos.y()));
      smooth_spread =
          std::max(smooth_spread, std::abs(smooth.Samples()[i].pos.y()));
    }
    QVERIFY(smooth_spread < raw_spread / 3);
  }

  void FinishReachesThePen() {
    PenStroke stroke(1.0, 0.5);
    for (int x = 0; x <= 100; ++x) {
      stroke.Add(QPointF(x, 0), 1.0F);
    }
    stroke.Finish(QPointF(100, 0), 1.0F);
    QCOMPARE(stroke.Samples().back().pos, QPointF(100, 0));
  }

  void CircleStaysRound() {
    PenStroke stroke(0.0, 0.5);
    constexpr double kRadius = 100.0;
    for (int i = 0; i <= 360; ++i) {
      const double a = i * std::numbers::pi / 180.0;
      stroke.Add(QPointF(std::cos(a), std::sin(a)) * kRadius, 1.0F);
    }
    const std::vector<StrokePoint> points = stroke.Simplify(1.0);
    QVERIFY(points.size() > 8);
    QVERIFY(points.size() < 60);
    for (const StrokePoint& point : points) {
      QVERIFY(!point.is_corner);
    }
  }
};

QTEST_GUILESS_MAIN(PenStrokeTest)
#include "tst_penstroke.moc"
