#include <QTest>

#include "document/ChannelManager.h"
#include "eval/Curves.h"

using snapper::Curve;
using snapper::Curves;
using snapper::Key;

namespace {

std::vector<Key> TwoKeys(Curve curve) {
  return {{.frame = 10, .value = QPointF(0, 0), .curve = curve},
          {.frame = 20, .value = QPointF(100, 50)}};
}

}  // namespace

class CurvesTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void NoKeysGiveTheSetupValue() {
    QCOMPARE(Curves::ValueAt({}, QPointF(3, 4), 50, 1), QPointF(3, 4));
  }

  void KeysHoldTheirValues() {
    const std::vector<Key> keys = TwoKeys(Curve::kLinear);
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 0, 1), QPointF());
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 10, 1), QPointF(0, 0));
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 20, 1), QPointF(100, 50));
    // Past the last key the value stays.
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 90, 1), QPointF(100, 50));
  }

  void LinearIsHalfWayInTheMiddle() {
    QCOMPARE(Curves::ValueAt(TwoKeys(Curve::kLinear), QPointF(), 15, 1),
             QPointF(50, 25));
  }

  void StepHoldsUntilTheNextKey() {
    const std::vector<Key> keys = TwoKeys(Curve::kStep);
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 19, 1), QPointF(0, 0));
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 20, 1), QPointF(100, 50));
  }

  void EaseInStartsSlowAndEaseOutEndsSlow() {
    const double in =
        Curves::ValueAt(TwoKeys(Curve::kEaseIn), QPointF(), 12, 1).x();
    const double out =
        Curves::ValueAt(TwoKeys(Curve::kEaseOut), QPointF(), 12, 1).x();
    QVERIFY(in < 20);
    QVERIFY(out > 20);
  }

  void SmoothPassesThroughEveryKey() {
    const std::vector<Key> keys = {{.frame = 5, .value = QPointF(10, 0)},
                                   {.frame = 15, .value = QPointF(-10, 0)},
                                   {.frame = 30, .value = QPointF(40, 0)}};
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 5, 1), QPointF(10, 0));
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 15, 1), QPointF(-10, 0));
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 30, 1), QPointF(40, 0));
  }

  void NoisyShakesBetweenKeysButHitsThem() {
    std::vector<Key> keys = TwoKeys(Curve::kNoisy);
    keys[0].param = 5;
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 20, 7), QPointF(100, 50));
    const QPointF middle = Curves::ValueAt(keys, QPointF(), 15, 7);
    QVERIFY(middle != QPointF(50, 25));
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 15, 7), middle);
  }

  void CycleLoopsBackToTheKeyBefore() {
    const std::vector<Key> keys = {
        {.frame = 10, .value = QPointF(0, 0), .curve = Curve::kLinear},
        {.frame = 20, .value = QPointF(100, 0), .curve = Curve::kCycle}};
    // Frame 25 plays like frame 15 again.
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 25, 1),
             Curves::ValueAt(keys, QPointF(), 15, 1));
    QCOMPARE(Curves::ValueAt(keys, QPointF(), 35, 1),
             Curves::ValueAt(keys, QPointF(), 15, 1));
  }

  void FileNamesRoundTrip() {
    for (int i = 0; i < snapper::kCurveCount; ++i) {
      const auto curve = static_cast<Curve>(i);
      QCOMPARE(Curves::FromFileName(Curves::FileName(curve)), curve);
    }
  }
};

QTEST_GUILESS_MAIN(CurvesTest)
#include "tst_curves.moc"
