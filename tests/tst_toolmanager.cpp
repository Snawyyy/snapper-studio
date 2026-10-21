#include <QTest>

#include "app/CommandRegistry.h"
#include "app/Theme.h"
#include "document/Document.h"
#include "edit/SelectionManager.h"
#include "edit/ToolManager.h"
#include "edit/UndoManager.h"

using snapper::ToolManager;

namespace {

class FakeTool : public snapper::Tool {
 public:
  FakeTool(snapper::ToolContext context, QString id, Qt::Key key)
      : Tool(std::move(context)), id_(std::move(id)), key_(key) {}

  snapper::ToolInfo Info() const override {
    return {.id = id_, .title = id_, .key = QKeySequence(key_), .hint = {}};
  }
  void Press(const snapper::ToolInput& /*input*/) override { busy_ = true; }
  void Release(const snapper::ToolInput& /*input*/) override { busy_ = false; }
  bool IsBusy() const override { return busy_; }

 private:
  QString id_;
  Qt::Key key_;
  bool busy_ = false;
};

struct Fixture {
  Fixture() {
    document.ResetToNew();
    const snapper::ToolContext context{.document = document,
                                       .undo = undo,
                                       .selection = selection,
                                       .theme = theme,
                                       .hint = [](const QString&) {}};
    tools.Add(
        std::make_unique<FakeTool>(context, QStringLiteral("pen"), Qt::Key_P));
    tools.Add(std::make_unique<FakeTool>(context, QStringLiteral("select"),
                                         Qt::Key_V));
    tools.RegisterCommands(commands);
  }

  snapper::Document document;
  snapper::UndoManager undo;
  snapper::SelectionManager selection{document};
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
