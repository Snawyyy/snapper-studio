#include <QLineF>
#include <QTest>

#include <numbers>

#include "document/Document.h"
#include "edit/AnimationEditor.h"
#include "edit/BoneCommands.h"
#include "edit/Playhead.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"
#include "eval/FrameEvaluator.h"

using snapper::Bone;
using snapper::ChannelProperty;
using snapper::Id;
using snapper::kNoId;

namespace {

bool Near(const QPointF& a, const QPointF& b) {
  return QLineF(a, b).length() < 1e-6;
}

// A bone layer holding the drawing, with a chain of three bones along x
// from (0, 0), each 10 long.
struct Fixture {
  Fixture() {
    document.ResetToNew();
    drawing = document.Layers().All()[0].id;
    bones = document.NewId();
    document.Layers().Insert({.id = bones, .type = snapper::LayerType::kBone},
                             0);
    document.Layers().Place(drawing, bones, 1);
    Id parent = kNoId;
    for (int i = 0; i < 3; ++i) {
      const Bone bone{.id = document.NewId(),
                      .layer = bones,
                      .parent = parent,
                      .transform = {.position = QPointF(i == 0 ? 0 : 10, 0)},
                      .length = 10};
      undo.Push(std::make_unique<snapper::AddBoneCommand>(document, bone));
      chain.push_back(bone.id);
      parent = bone.id;
    }
  }

  snapper::Pose Setup() const {
    return snapper::FrameEvaluator::Evaluate(document, 0);
  }

  snapper::Document document;
  snapper::UndoManager undo;
  snapper::Playhead playhead{document};
  snapper::AnimationEditor animation{document, playhead};
  snapper::SelectionManager selection{document};
  Id drawing = kNoId;
  Id bones = kNoId;
  std::vector<Id> chain;
};

}  // namespace

class BoneCommandsTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void OnTheSetupFrameBonesChangeThemselves() {
    Fixture f;
    f.undo.Push(f.animation.PlaceBones(
        {{.bone = f.chain[0], .transform = {.angle = 1.0}}}, 0));
    QCOMPARE(f.document.Skeletons().Find(f.chain[0])->transform.angle, 1.0);
    QCOMPARE(f.document.Channels().Count(), 0);
    f.undo.Undo();
    QCOMPARE(f.document.Skeletons().Find(f.chain[0])->transform.angle, 0.0);
  }

  void OnOtherFramesOnlyWhatChangedGetsAKey() {
    Fixture f;
    f.playhead.SetFrame(5);
    f.undo.Push(f.animation.PlaceBones(
        {{.bone = f.chain[1],
          .transform = {.position = QPointF(10, 0), .angle = 0.5}}},
        0));
    QVERIFY(f.document.Channels().Find(f.chain[1],
                                       ChannelProperty::kBoneAngle) != nullptr);
    QVERIFY(f.document.Channels().Find(
                f.chain[1], ChannelProperty::kBonePosition) == nullptr);
    QCOMPARE(f.document.Skeletons().Find(f.chain[1])->transform.angle, 0.0);
  }

  void DeletingABoneKeepsItsChildWhereItWas() {
    Fixture f;
    f.document.Skeletons().Set([&] {
      Bone turned = *f.document.Skeletons().Find(f.chain[0]);
      turned.transform.angle = std::numbers::pi / 2;
      return turned;
    }());
    const QPointF tip = f.Setup().FindBone(f.chain[2])->Tip();
    f.document.Bindings().BindLayer(f.drawing, f.chain[1]);
    f.undo.Push(snapper::MakeRemoveBones(f.document, {f.chain[1]}));
    QCOMPARE(f.document.Skeletons().Count(), 2);
    QCOMPARE(f.document.Skeletons().Find(f.chain[2])->parent, f.chain[0]);
    QVERIFY(Near(f.Setup().FindBone(f.chain[2])->Tip(), tip));
    QCOMPARE(f.document.Bindings().BoneOfLayer(f.drawing), kNoId);
    f.undo.Undo();
    QCOMPARE(f.document.Skeletons().Find(f.chain[2])->parent, f.chain[1]);
    QCOMPARE(f.document.Bindings().BoneOfLayer(f.drawing), f.chain[1]);
  }

  void BindingUndoes() {
    Fixture f;
    f.undo.Push(std::make_unique<snapper::BindLayerCommand>(
        f.document, f.drawing, f.chain[0]));
    QCOMPARE(f.document.Bindings().BoneOfLayer(f.drawing), f.chain[0]);
    f.undo.Undo();
    QCOMPARE(f.document.Bindings().BoneOfLayer(f.drawing), kNoId);
  }

  void BoneSelectionStaysOnItsBoneLayer() {
    Fixture f;
    f.selection.SetActiveLayer(f.drawing);
    QCOMPARE(f.selection.BoneLayer(), f.bones);
    f.selection.SetBones({f.chain[0]});
    f.selection.SetActiveLayer(f.bones);
    QCOMPARE(f.selection.Bones().size(), 1);
    f.document.Layers().Place(f.drawing, kNoId, 2);
    f.selection.SetActiveLayer(f.drawing);
    QVERIFY(f.selection.Bones().isEmpty());
  }
};

QTEST_MAIN(BoneCommandsTest)
#include "tst_bonecommands.moc"
