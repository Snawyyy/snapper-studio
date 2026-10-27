#include <QSignalSpy>
#include <QTest>

#include "document/Document.h"

using snapper::Id;
using snapper::kNoId;

class BindingManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void BindingReplacesAndUnbinds() {
    snapper::BindingManager bindings;
    const QSignalSpy spy(&bindings,
                         &snapper::BindingManager::LayerBindingChanged);
    QVERIFY(bindings.BindLayer(1, 7));
    QVERIFY(bindings.BindLayer(1, 7));
    QVERIFY(bindings.BindLayer(2, 7));
    QVERIFY(bindings.BindLayer(1, 8));
    QCOMPARE(bindings.BoneOfLayer(1), 8);
    QCOMPARE(bindings.LayersOnBone(7), std::vector<Id>{2});
    QVERIFY(bindings.BindLayer(1, kNoId));
    QCOMPARE(bindings.BoneOfLayer(1), kNoId);
    QVERIFY(!bindings.BindLayer(kNoId, 7));
    QCOMPARE(spy.count(), 4);
  }

  void LayersBindOnlyToBonesOfTheirBoneLayer() {
    snapper::Document document;
    document.ResetToNew();
    const Id drawing = document.Layers().All()[0].id;
    const Id bones = document.NewId();
    const Id other_bones = document.NewId();
    document.Layers().Insert({.id = bones, .type = snapper::LayerType::kBone},
                             0);
    document.Layers().Place(drawing, bones, 1);
    document.Layers().Insert(
        {.id = other_bones, .type = snapper::LayerType::kBone}, 2);
    document.Skeletons().Insert({.id = 50, .layer = bones}, 0);
    document.Skeletons().Insert({.id = 51, .layer = other_bones}, 1);
    QVERIFY(document.CanBind(drawing, 50));
    QVERIFY(!document.CanBind(drawing, 51));
    QVERIFY(!document.CanBind(other_bones, 51));
    QVERIFY(!document.CanBind(drawing, 99));
  }
};

QTEST_GUILESS_MAIN(BindingManagerTest)
#include "tst_bindingmanager.moc"
