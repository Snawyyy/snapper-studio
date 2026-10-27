#include <QLineF>
#include <QTest>

#include <numbers>

#include "document/Document.h"
#include "eval/FrameEvaluator.h"

using snapper::Bone;
using snapper::ChannelProperty;
using snapper::Id;
using snapper::Pose;

namespace {

bool Near(const QPointF& a, const QPointF& b) {
  return QLineF(a, b).length() < 1e-6;
}

// A bone layer at the bottom holding the drawing, with an upper arm along
// x from (100, 100) and a forearm at its tip. The drawing follows the
// forearm.
struct Rig {
  Rig() {
    document.ResetToNew();
    drawing = document.Layers().All()[0].id;
    bones = document.NewId();
    document.Layers().Insert({.id = bones, .type = snapper::LayerType::kBone},
                             0);
    document.Layers().Place(drawing, bones, 1);
    upper = document.NewId();
    fore = document.NewId();
    document.Skeletons().Insert({.id = upper,
                                 .layer = bones,
                                 .transform = {.position = QPointF(100, 100)},
                                 .length = 50},
                                0);
    document.Skeletons().Insert({.id = fore,
                                 .layer = bones,
                                 .parent = upper,
                                 .transform = {.position = QPointF(50, 0)},
                                 .length = 40},
                                1);
    document.Bindings().BindLayer(drawing, fore);
  }

  snapper::Document document;
  Id drawing = snapper::kNoId;
  Id bones = snapper::kNoId;
  Id upper = snapper::kNoId;
  Id fore = snapper::kNoId;
};

}  // namespace

class FrameEvaluatorTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void ChildrenSitAtTheirParentsTip() {
    const Rig rig;
    const Pose pose = snapper::FrameEvaluator::Evaluate(rig.document, 0);
    QVERIFY(Near(pose.FindBone(rig.fore)->Base(), QPointF(150, 100)));
    QVERIFY(Near(pose.FindBone(rig.fore)->Tip(), QPointF(190, 100)));
    QVERIFY(pose.LayerMatrix(rig.drawing).isIdentity());
  }

  void TurningTheParentCarriesTheChildAndTheBoundLayer() {
    Rig rig;
    rig.document.Channels().Put(
        {.target = rig.upper,
         .property = ChannelProperty::kBoneAngle,
         .keys = {{.frame = 1, .value = QPointF(std::numbers::pi / 2, 0)}}});
    const Pose pose = snapper::FrameEvaluator::Evaluate(rig.document, 1);
    QVERIFY(Near(pose.FindBone(rig.fore)->Base(), QPointF(100, 150)));
    // A spot on the forearm in the setup pose stays on it.
    QVERIFY(Near(pose.LayerMatrix(rig.drawing).map(QPointF(170, 100)),
                 QPointF(100, 170)));
  }

  void ScaleStretchesAlongTheBoneAndMovesChildren() {
    Rig rig;
    rig.document.Channels().Put(
        {.target = rig.upper,
         .property = ChannelProperty::kBoneScale,
         .keys = {{.frame = 1, .value = QPointF(2, 0)}}});
    const Pose pose = snapper::FrameEvaluator::Evaluate(rig.document, 1);
    QVERIFY(Near(pose.FindBone(rig.upper)->Tip(), QPointF(200, 100)));
    QVERIFY(Near(pose.FindBone(rig.fore)->Base(), QPointF(200, 100)));
  }

  void ALayerOutsideTheBoneLayerIgnoresItsBinding() {
    Rig rig;
    rig.document.Layers().Place(rig.drawing, snapper::kNoId, 0);
    rig.document.Channels().Put(
        {.target = rig.fore,
         .property = ChannelProperty::kBoneAngle,
         .keys = {{.frame = 1, .value = QPointF(1, 0)}}});
    const Pose pose = snapper::FrameEvaluator::Evaluate(rig.document, 1);
    QVERIFY(pose.LayerMatrix(rig.drawing).isIdentity());
  }

  void ATrialPoseShowsOnlyOnTheSetupFrame() {
    Rig rig;
    QHash<Id, snapper::BoneTransform> trial;
    trial.insert(rig.upper,
                 {.position = QPointF(100, 100), .angle = std::numbers::pi});
    const Pose pose = snapper::FrameEvaluator::Evaluate(rig.document, 0, trial);
    QVERIFY(Near(pose.FindBone(rig.upper)->Tip(), QPointF(50, 100)));
    QCOMPARE(rig.document.Skeletons().Find(rig.upper)->transform.angle, 0.0);
  }
};

QTEST_GUILESS_MAIN(FrameEvaluatorTest)
#include "tst_frameevaluator.moc"
