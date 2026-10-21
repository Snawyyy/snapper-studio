#include <QSignalSpy>
#include <QTest>

#include "document/LayerManager.h"

using snapper::Layer;
using snapper::LayerManager;

namespace {

Layer Named(snapper::Id id, const char* name) {
  return Layer{.id = id, .name = QString::fromLatin1(name)};
}

}  // namespace

class LayerManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void InsertKeepsOrderBottomToTop() {
    LayerManager layers;
    QVERIFY(layers.Insert(Named(1, "a"), 0));
    QVERIFY(layers.Insert(Named(2, "b"), 1));
    QVERIFY(layers.Insert(Named(3, "c"), 1));
    QCOMPARE(layers.IndexOf(1), 0);
    QCOMPARE(layers.IndexOf(3), 1);
    QCOMPARE(layers.IndexOf(2), 2);
  }

  void RejectsBadIds() {
    LayerManager layers;
    QVERIFY(!layers.Insert(Named(snapper::kNoId, "none"), 0));
    QVERIFY(layers.Insert(Named(1, "a"), 0));
    QVERIFY(!layers.Insert(Named(1, "again"), 0));
    QCOMPARE(layers.Count(), 1);
  }

  void StopsAtTheLimit() {
    LayerManager layers;
    for (int i = 1; i <= LayerManager::kMaxLayers; ++i) {
      QVERIFY(layers.Insert(Named(i, "x"), i));
    }
    QVERIFY(!layers.Insert(Named(LayerManager::kMaxLayers + 1, "x"), 0));
  }

  void TakeReturnsWhereItWas() {
    LayerManager layers;
    layers.Insert(Named(1, "a"), 0);
    layers.Insert(Named(2, "b"), 1);
    const snapper::RemovedLayer removed =
        layers.Take(1).value_or(snapper::RemovedLayer{.index = -1});
    QCOMPARE(removed.index, 0);
    QCOMPARE(removed.layer.name, QStringLiteral("a"));
    QCOMPARE(layers.Count(), 1);
    QVERIFY(!layers.Take(1).has_value());
  }

  void EditsAnnounceTheLayer() {
    LayerManager layers;
    layers.Insert(Named(1, "a"), 0);
    const QSignalSpy spy(&layers, &LayerManager::LayerChanged);
    QVERIFY(layers.SetName(1, QStringLiteral("Head")));
    QVERIFY(layers.SetVisible(1, false));
    QVERIFY(!layers.SetLocked(7, true));
    QCOMPARE(spy.count(), 2);
    QCOMPARE(layers.Find(1)->name, QStringLiteral("Head"));
    QVERIFY(!layers.Find(1)->visible);
  }
};

QTEST_GUILESS_MAIN(LayerManagerTest)
#include "tst_layermanager.moc"
