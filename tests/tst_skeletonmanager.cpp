#include <QSignalSpy>
#include <QTest>

#include "document/SkeletonManager.h"

using snapper::Bone;
using snapper::Id;
using snapper::SkeletonManager;

namespace {

constexpr Id kLayer = 10;
constexpr Id kOtherLayer = 11;

Bone BoneOf(Id id, Id parent = snapper::kNoId, Id layer = kLayer) {
  return Bone{.id = id, .layer = layer, .parent = parent};
}

}  // namespace

class SkeletonManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void ParentsComeBeforeChildrenOnTheSameLayer() {
    SkeletonManager bones;
    QVERIFY(bones.Insert(BoneOf(1), 0));
    QVERIFY(!bones.Insert(BoneOf(2, 1), 0));
    QVERIFY(!bones.Insert(BoneOf(2, 1, kOtherLayer), 1));
    QVERIFY(!bones.Insert(BoneOf(2, 9), 1));
    QVERIFY(bones.Insert(BoneOf(2, 1), 1));
    QVERIFY(!bones.Insert(BoneOf(2), 2));
    QVERIFY(!bones.Insert(BoneOf(snapper::kNoId), 2));
    QCOMPARE(bones.ChildrenOf(1), std::vector<Id>{2});
    QVERIFY(bones.IsUnder(2, 1));
    QVERIFY(!bones.IsUnder(1, 2));
  }

  void TakeKeepsChildrenSafe() {
    SkeletonManager bones;
    bones.Insert(BoneOf(1), 0);
    bones.Insert(BoneOf(2, 1), 1);
    QVERIFY(!bones.Take(1).has_value());
    const auto removed = bones.Take(2);
    QVERIFY(removed.has_value());
    QCOMPARE(removed->index, 1);
    QVERIFY(bones.Take(1).has_value());
    QCOMPARE(bones.Count(), 0);
  }

  void SetKeepsIdAndLayerAndRefusesLoops() {
    SkeletonManager bones;
    bones.Insert(BoneOf(1), 0);
    bones.Insert(BoneOf(2, 1), 1);
    const QSignalSpy spy(&bones, &SkeletonManager::BonesChanged);
    Bone changed = *bones.Find(2);
    changed.name = QStringLiteral("forearm");
    changed.layer = kOtherLayer;
    changed.length = 0;
    QVERIFY(bones.Set(changed));
    QCOMPARE(bones.Find(2)->layer, kLayer);
    QCOMPARE(bones.Find(2)->length, SkeletonManager::kMinLength);
    Bone loop = *bones.Find(1);
    loop.parent = 2;
    QVERIFY(!bones.Set(loop));
    QCOMPARE(spy.count(), 1);
  }

  void StopsAtTheSkeletonLimit() {
    SkeletonManager bones;
    for (int i = 1; i <= SkeletonManager::kMaxBonesPerSkeleton; ++i) {
      QVERIFY(bones.Insert(BoneOf(i), i));
    }
    QVERIFY(!bones.Insert(BoneOf(9999), 0));
    QVERIFY(bones.Insert(BoneOf(9999, snapper::kNoId, kOtherLayer), 0));
  }
};

QTEST_GUILESS_MAIN(SkeletonManagerTest)
#include "tst_skeletonmanager.moc"
