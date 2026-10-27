#include <QTest>

#include "app/CommandRegistry.h"
#include "app/Theme.h"
#include "document/Document.h"
#include "edit/AnimationEditor.h"
#include "edit/LookManager.h"
#include "edit/Playhead.h"
#include "edit/SelectionManager.h"
#include "edit/ToolManager.h"
#include "edit/UndoManager.h"
#include "eval/FrameEvaluator.h"

using snapper::ToolManager;

namespace {

class FakeTool : public snapper::Tool {
 public:
  FakeTool(snapper::ToolContext context, QString id, Qt::Key key,
           bool is_for_bones = false)
      : Tool(std::move(context)),
        id_(std::move(id)),
        key_(key),
        is_for_bones_(is_for_bones) {}

  snapper::ToolInfo Info() const override {
    return {.id = id_,
            .title = id_,
            .key = QKeySequence(key_),
            .hint = {},
            .is_for_bone_layers = is_for_bones_};
  }
  void Press(const snapper::ToolInput& /*input*/) override { busy_ = true; }
  void Release(const snapper::ToolInput& /*input*/) override { busy_ = false; }
  bool IsBusy() const override { return busy_; }

 private:
  QString id_;
  Qt::Key key_;
  bool is_for_bones_;
  bool busy_ = false;
};

struct Fixture {
  Fixture() {
    document.ResetToNew();
    tools.Add(std::make_unique<FakeTool>(Context(), QStringLiteral("pen"),
                                         Qt::Key_P));
    tools.Add(std::make_unique<FakeTool>(Context(), QStringLiteral("select"),
                                         Qt::Key_V));
    tools.RegisterCommands(commands);
  }

  snapper::ToolContext Context() {
    return {.document = document,
            .undo = undo,
            .selection = selection,
            .looks = looks,
            .frames = frames,
            .animation = animation,
            .theme = theme,
            .hint = [](const QString&) {}};
  }

  snapper::Document document;
  snapper::UndoManager undo;
  snapper::SelectionManager selection{document};
  snapper::LookManager looks{document, selection, undo, [](const QString&) {}};
  snapper::Playhead playhead{document};
  snapper::FrameEvaluator frames{document, playhead};
  snapper::AnimationEditor animation{document, playhead};
  snapper::CommandRegistry commands;
  snapper::Theme theme;
  ToolManager tools;
};

}  // namespace

class ToolManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void TapSwitchesForGood() {
    Fixture f;
    f.commands.Run(QStringLiteral("tool.select"));
    f.tools.KeyReleased(Qt::Key_V);
    QCOMPARE(f.tools.ActiveId(), QStringLiteral("select"));
  }

  void HoldReturnsToTheToolBefore() {
    Fixture f;
    f.commands.Run(QStringLiteral("tool.select"));
    QTest::qWait(ToolManager::kHoldMs + 50);
    f.tools.KeyReleased(Qt::Key_V);
    QCOMPARE(f.tools.ActiveId(), QStringLiteral("pen"));
  }

  void HoldWaitsForTheStrokeToEnd() {
    Fixture f;
    f.commands.Run(QStringLiteral("tool.select"));
    f.tools.Press({});
    QTest::qWait(ToolManager::kHoldMs + 50);
    f.tools.KeyReleased(Qt::Key_V);
    QCOMPARE(f.tools.ActiveId(), QStringLiteral("select"));
    f.tools.Release({});
    QCOMPARE(f.tools.ActiveId(), QStringLiteral("pen"));
  }

  void ASharedKeyPicksTheToolForTheLayer() {
    Fixture f;
    ToolManager tools;
    snapper::CommandRegistry commands;
    bool is_bone_layer = false;
    tools.Add(std::make_unique<FakeTool>(f.Context(), QStringLiteral("pen"),
                                         Qt::Key_P));
    tools.Add(std::make_unique<FakeTool>(f.Context(), QStringLiteral("layer"),
                                         Qt::Key_T));
    tools.Add(std::make_unique<FakeTool>(f.Context(), QStringLiteral("bone"),
                                         Qt::Key_T, true));
    tools.SetBoneLayerCheck([&] { return is_bone_layer; });
    tools.RegisterCommands(commands);
    commands.Run(QStringLiteral("tool.layer"));
    QCOMPARE(tools.ActiveId(), QStringLiteral("layer"));
    tools.Activate(QStringLiteral("pen"));
    is_bone_layer = true;
    commands.Run(QStringLiteral("tool.layer"));
    QCOMPARE(tools.ActiveId(), QStringLiteral("bone"));
    QVERIFY(commands.Find(QStringLiteral("tool.bone"))->shortcut.isEmpty());
  }

  void HoldingTheActiveToolKeyChangesNothing() {
    Fixture f;
    f.commands.Run(QStringLiteral("tool.pen"));
    QTest::qWait(ToolManager::kHoldMs + 50);
    f.tools.KeyReleased(Qt::Key_P);
    QCOMPARE(f.tools.ActiveId(), QStringLiteral("pen"));
  }
};

QTEST_MAIN(ToolManagerTest)
#include "tst_toolmanager.moc"
