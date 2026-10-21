#include <QTest>

// Proves the test setup runs. Replaced by real manager tests as they land.
class SmokeTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void Runs() { QVERIFY(true); }
};

QTEST_MAIN(SmokeTest)
#include "tst_smoke.moc"
