#include <QSignalSpy>
#include <QTest>

#include "document/LayerManager.h"

using snapper::Layer;
using snapper::LayerManager;

namespace {

Layer Named(snapper::Id id, const char* name) {
  return Layer{.id = id, .name = QString::fromLatin1(name)};
}

Layer Bones(snapper::Id id) {
  return Layer{.id = id, .type = snapper::LayerType::kBone};
}

Layer Inside(snapper::Id id, snapper::Id parent) {
  return Layer{.id = id, .parent = parent};
}

// 1 vector, 2 bones holding 3 and 4, 5 vector: bottom to top.
void FillTree(LayerManager& layers) {
  layers.Insert(Named(1, "a"), 0);
  layers.Insert(Bones(2), 1);
  layers.Insert(Inside(3, 2), 2);
  layers.Insert(Inside(4, 2), 3);
  layers.Insert(Named(5, "e"), 4);
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

  void LayersSitOnlyInsideBoneLayersInOneBlock() {
    LayerManager layers;
    FillTree(layers);
    QVERIFY(!layers.Insert(Inside(6, 1), 1));
    QVERIFY(!layers.Insert(Inside(6, 2), 0));
    QVERIFY(!layers.Insert(Inside(6, 2), 5));
    QVERIFY(!layers.Insert(Named(6, "f"), 3));
    QVERIFY(layers.Insert(Inside(6, 2), 4));
    QCOMPARE(layers.EndOf(2), 5);
    QVERIFY(layers.IsInside(6, 2));
    QCOMPARE(layers.DepthOf(6), 1);
    QCOMPARE(layers.BoneLayerOf(6), 2);
    QCOMPARE(layers.BoneLayerOf(1), snapper::kNoId);
  }

  void HidingAndLockingReachInside() {
    LayerManager layers;
    FillTree(layers);
    layers.SetVisible(2, false);
    layers.SetLocked(2, true);
    QVERIFY(!layers.IsShown(3));
    QVERIFY(layers.IsLocked(4));
    QVERIFY(layers.IsShown(5));
    QVERIFY(!layers.IsLocked(1));
  }

  void TopFirstPutsHoldersAboveWhatTheyHold() {
    LayerManager layers;
    FillTree(layers);
    const std::vector<int> expected = {4, 1, 3, 2, 0};
    QCOMPARE(layers.TopFirst(), expected);
  }

  void TakeTreeTakesTheBlockAndPutsItBack() {
    LayerManager layers;
    FillTree(layers);
    QVERIFY(!layers.Take(2).has_value());
    const std::vector<snapper::RemovedLayer> removed = layers.TakeTree(2);
    QCOMPARE(removed.size(), size_t{3});
    QCOMPARE(layers.Count(), 2);
    for (const snapper::RemovedLayer& each : removed) {
      QVERIFY(layers.Insert(each.layer, each.index));
    }
    QCOMPARE(layers.IndexOf(4), 3);
  }

  void PlaceMovesBlocksInAndOut() {
    LayerManager layers;
    FillTree(layers);
    // 5 into the bone layer, on top.
    QVERIFY(layers.Place(5, 2, 4));
    QCOMPARE(layers.Find(5)->parent, 2);
    QCOMPARE(layers.EndOf(2), 5);
    // The bone layer can't go inside itself, or split another block.
    QVERIFY(!layers.Place(2, 2, 1));
    QVERIFY(!layers.Place(1, snapper::kNoId, 2));
    // The whole bone layer below 1.
    QVERIFY(layers.Place(2, snapper::kNoId, 0));
    QCOMPARE(layers.IndexOf(1), 4);
    QCOMPARE(layers.IndexOf(5), 3);
  }

  void StepIndexSkipsWholeBlocks() {
    LayerManager layers;
    FillTree(layers);
    QVERIFY(layers.Place(1, snapper::kNoId, *layers.StepIndex(1, 1)));
    QCOMPARE(layers.IndexOf(1), 3);
    QCOMPARE(layers.IndexOf(2), 0);
    QVERIFY(!layers.StepIndex(5, 1).has_value());
    QVERIFY(!layers.StepIndex(4, 1).has_value());
    QVERIFY(layers.Place(4, 2, *layers.StepIndex(4, -1)));
    QCOMPARE(layers.IndexOf(4), 1);
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
