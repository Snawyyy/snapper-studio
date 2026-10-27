#include <QLineF>
#include <QTest>
#include <QUndoStack>

#include <cmath>

#include "app/Theme.h"
#include "document/Document.h"
#include "edit/AnimationEditor.h"
#include "edit/LookManager.h"
#include "edit/Playhead.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"
#include "edit/tools/AddBoneTool.h"
#include "edit/tools/BindLayerTool.h"
#include "edit/tools/ManipulateBonesTool.h"
#include "edit/tools/SelectBoneTool.h"
#include "edit/tools/TransformBoneTool.h"
#include "eval/FrameEvaluator.h"
#include "render/ViewTransform.h"

using snapper::ChannelProperty;
using snapper::Id;
using snapper::kNoId;

namespace {

bool Near(const QPointF& a, const QPointF& b, double slack = 0.5) {
  return QLineF(a, b).length() < slack;
}

// A 200 by 200 view at 100%, with a bone layer holding the drawing.
struct Fixture {
  Fixture() {
    document.ResetToNew();
    view.SetViewSize(QSizeF(200, 200));
    view.Fit(QRectF(0, 0, 200, 200), 0);
    drawing = document.Layers().All()[0].id;
    bones = document.NewId();
    document.Layers().Insert({.id = bones, .type = snapper::LayerType::kBone},
                             0);
    document.Layers().Place(drawing, bones, 1);
    selection.SetActiveLayer(bones);
  }

  snapper::ToolContext Context() {
    return {.document = document,
            .undo = undo,
            .selection = selection,
            .looks = looks,
            .frames = frames,
            .animation = animation,
            .theme = theme,
            .hint = [this](const QString& text) { hint = text; }};
  }

  snapper::ToolInput At(QPointF pos, Qt::KeyboardModifiers modifiers = {}) {
    return {.canvas = view.ToCanvas(pos),
            .screen = pos,
            .modifiers = modifiers,
            .view = &view};
  }

  void Drag(snapper::Tool& tool, QPointF from, QPointF to,
            Qt::KeyboardModifiers modifiers = {}) {
    tool.Press(At(from, modifiers));
    for (int step = 1; step <= 8; ++step) {
      tool.Drag(At(from + (to - from) * step / 8.0, modifiers));
    }
    tool.Release(At(to, modifiers));
  }

  // Two bones along x: (20, 100) to (70, 100) to (120, 100).
  std::vector<Id> AddArm() {
    snapper::AddBoneTool add(Context());
    Drag(add, QPointF(20, 100), QPointF(70, 100));
    Drag(add, QPointF(70, 100), QPointF(120, 100));
    std::vector<Id> arm;
    for (const snapper::Bone& bone : document.Skeletons().All()) {
      arm.push_back(bone.id);
    }
    return arm;
  }

  QPointF TipOf(Id bone) { return frames.Current().FindBone(bone)->Tip(); }

  snapper::Document document;
  snapper::UndoManager undo;
  snapper::SelectionManager selection{document};
  snapper::LookManager looks{document, selection, undo, [](const QString&) {}};
  snapper::Playhead playhead{document};
  snapper::FrameEvaluator frames{document, playhead};
  snapper::AnimationEditor animation{document, playhead};
  snapper::Theme theme;
  snapper::ViewTransform view;
  QString hint;
  Id drawing = kNoId;
  Id bones = kNoId;
};

}  // namespace

class BoneToolsTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void AddingGrowsAChainFromTheSelectedTip() {
    Fixture f;
    const std::vector<Id> arm = f.AddArm();
    QCOMPARE(arm.size(), size_t{2});
    const snapper::Bone* fore = f.document.Skeletons().Find(arm[1]);
    QCOMPARE(fore->parent, arm[0]);
    QVERIFY(Near(fore->transform.position, QPointF(50, 0), 1e-6));
    QVERIFY(Near(f.TipOf(arm[1]), QPointF(120, 100), 1e-6));
    QCOMPARE(f.selection.Bones(), QSet<Id>{arm[1]});
  }

  void BonesAreAddedOnlyOnTheSetupFrame() {
    Fixture f;
    f.playhead.SetFrame(3);
    snapper::AddBoneTool add(f.Context());
    f.Drag(add, QPointF(20, 100), QPointF(70, 100));
    QCOMPARE(f.document.Skeletons().Count(), 0);
    QVERIFY(f.hint.contains(QStringLiteral("setup")));
  }

  void PullingATipBendsTheChainAndSetsKeys() {
    Fixture f;
    const std::vector<Id> arm = f.AddArm();
    f.playhead.SetFrame(5);
    snapper::ManipulateBonesTool pull(f.Context());
    f.Drag(pull, QPointF(120, 100), QPointF(90, 140));
    QVERIFY(Near(f.TipOf(arm[1]), QPointF(90, 140), 1.0));
    QVERIFY(f.document.Channels().Find(arm[0], ChannelProperty::kBoneAngle) !=
            nullptr);
    QCOMPARE(f.document.Skeletons().Find(arm[0])->transform.angle, 0.0);
  }

  void APinnedBoneStaysPut() {
    Fixture f;
    const std::vector<Id> arm = f.AddArm();
    snapper::Bone upper = *f.document.Skeletons().Find(arm[0]);
    upper.pinned = true;
    f.document.Skeletons().Set(upper);
    f.playhead.SetFrame(5);
    snapper::ManipulateBonesTool pull(f.Context());
    f.Drag(pull, QPointF(120, 100), QPointF(70, 150));
    QVERIFY(Near(f.TipOf(arm[0]), QPointF(70, 100), 1e-6));
    QVERIFY(Near(f.TipOf(arm[1]), QPointF(70, 150), 1.0));
  }

  void OnTheSetupFrameAPullIsOnlyATest() {
    Fixture f;
    const std::vector<Id> arm = f.AddArm();
    const int steps = f.undo.Stack()->count();
    snapper::ManipulateBonesTool pull(f.Context());
    pull.Press(f.At(QPointF(120, 100)));
    pull.Drag(f.At(QPointF(90, 140)));
    QVERIFY(Near(f.TipOf(arm[1]), QPointF(90, 140), 1.0));
    pull.Release(f.At(QPointF(90, 140)));
    QVERIFY(Near(f.TipOf(arm[1]), QPointF(120, 100), 1e-6));
    QVERIFY(f.frames.Trial().isEmpty());
    QCOMPARE(f.document.Channels().Count(), 0);
    QCOMPARE(f.undo.Stack()->count(), steps);
  }

  void TurningByTheTipOnTheSetupFrameSetsAngleAndLength() {
    Fixture f;
    const std::vector<Id> arm = f.AddArm();
    snapper::TransformBoneTool transform(f.Context());
    f.Drag(transform, QPointF(118, 100), QPointF(70, 150));
    const snapper::Bone* fore = f.document.Skeletons().Find(arm[1]);
    QVERIFY(std::abs(fore->transform.angle - M_PI / 2) < 1e-3);
    // Pressed 48 from the base and let go 50 from it.
    QVERIFY(std::abs(fore->length - 50.0 * 50 / 48) < 1e-3);
    f.undo.Undo();
    QCOMPARE(f.document.Skeletons().Find(arm[1])->transform.angle, 0.0);
  }

  void MovingABoneOnALaterFrameKeysItsPosition() {
    Fixture f;
    const std::vector<Id> arm = f.AddArm();
    f.playhead.SetFrame(2);
    snapper::TransformBoneTool transform(f.Context());
    f.Drag(transform, QPointF(45, 100), QPointF(45, 120));
    QVERIFY(Near(f.frames.Current().FindBone(arm[0])->Base(), QPointF(20, 120),
                 1e-6));
    QVERIFY(f.document.Channels().Find(
                arm[0], ChannelProperty::kBonePosition) != nullptr);
    QVERIFY(f.document.Channels().Find(arm[0], ChannelProperty::kBoneAngle) ==
            nullptr);
  }

  void BindingFollowsTheClickedBone() {
    Fixture f;
    const std::vector<Id> arm = f.AddArm();
    f.selection.SetActiveLayer(f.drawing);
    snapper::BindLayerTool bind(f.Context());
    bind.Press(f.At(QPointF(100, 100)));
    QCOMPARE(f.document.Bindings().BoneOfLayer(f.drawing), arm[1]);
    bind.Press(f.At(QPointF(100, 10)));
    QCOMPARE(f.document.Bindings().BoneOfLayer(f.drawing), kNoId);
  }

  void SelectingStepsAlongTheChain() {
    Fixture f;
    const std::vector<Id> arm = f.AddArm();
    snapper::SelectBoneTool select(f.Context());
    select.Press(f.At(QPointF(40, 100)));
    QCOMPARE(f.selection.Bones(), QSet<Id>{arm[0]});
    QVERIFY(select.KeyPress(Qt::Key_Down, {}));
    QCOMPARE(f.selection.Bones(), QSet<Id>{arm[1]});
    select.Press(f.At(QPointF(40, 100), Qt::ShiftModifier));
    QCOMPARE(f.selection.Bones().size(), 2);
  }
};

QTEST_MAIN(BoneToolsTest)
#include "tst_bonetools.moc"
