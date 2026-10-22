#include <QTest>
#include <QUndoStack>

#include "TestStyles.h"
#include "document/Document.h"
#include "edit/LookManager.h"
#include "edit/SelectionManager.h"
#include "edit/StyleActions.h"
#include "edit/StyleCommands.h"
#include "edit/UndoManager.h"

using snapper::Id;

namespace {

struct Fixture {
  Fixture() { document.ResetToNew(); }

  Id AddLine() {
    snapper::Shape shape;
    shape.id = document.NewId();
    shape.layer = document.Layers().All().front().id;
    for (int i = 0; i < 2; ++i) {
      shape.points.push_back({.id = document.NewId(), .pos = QPointF(i, 0)});
    }
    document.Shapes().Insert(shape, document.Shapes().Count());
    return shape.id;
  }

  snapper::Document document;
  snapper::UndoManager undo;
  snapper::SelectionManager selection{document};
  snapper::LookManager looks{document, selection, undo, [](const QString&) {}};
  snapper::StyleActions actions{document, looks, undo, [](const QString&) {}};
};

}  // namespace

class StyleActionsTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  // The selection takes the new style in the same undo step.
  void NewStyleGoesToTheSelection() {
    Fixture f;
    const Id line = f.AddLine();
    f.selection.SetPoints({f.document.Shapes().Find(line)->points[0].id});
    const Id style = f.actions.NewStyle();
    QVERIFY(style != snapper::kNoId);
    QCOMPARE(f.document.Shapes().Find(line)->styles[0], style);
    QCOMPARE(f.undo.Stack()->count(), 1);
    f.undo.Undo();
    QVERIFY(f.document.Styles().Find(style) == nullptr);
    QVERIFY(f.document.Shapes().Find(line)->styles[0] != style);
  }

  void NewStyleWithoutSelectionGoesToNewShapes() {
    Fixture f;
    const Id style = f.actions.NewStyle();
    QCOMPARE(f.looks.NewShapes().styles[0], style);
    QCOMPARE(f.document.Styles().Find(style)->name, QStringLiteral("Style 1"));
  }

  // Changing one style changes every shape that uses it.
  void EditingAStyleRestylesItsShapes() {
    Fixture f;
    const Id ink = snapper::test::AddInkStyle(f.document);
    std::vector<Id> lines;
    lines.reserve(3);
    for (int i = 0; i < 3; ++i) {
      lines.push_back(f.AddLine());
    }
    for (const Id id : lines) {
      snapper::ShapeLookState state =
          snapper::LookStatesOf(f.document.Shapes(), {id}).front();
      state.styles[0] = ink;
      f.document.Shapes().SetLooks({state});
    }
    f.actions.EditStyle(
        ink, [](snapper::Style& style) { style.look.line = Qt::red; });
    for (const Id id : lines) {
      const snapper::Shape* shape = f.document.Shapes().Find(id);
      QCOMPARE(f.document.Styles().Resolve(shape->look, shape->styles).line,
               QColor(Qt::red));
    }
    QVERIFY(f.actions.DeleteEffect(ink).contains(QStringLiteral("3 shapes")));
  }

  void RenameIgnoresBlankNames() {
    Fixture f;
    const Id ink = snapper::test::AddInkStyle(f.document);
    f.actions.RenameStyle(ink, QStringLiteral("  "));
    QVERIFY(!f.undo.CanUndo());
    f.actions.RenameStyle(ink, QStringLiteral(" Skin "));
    QCOMPARE(f.document.Styles().Find(ink)->name, QStringLiteral("Skin"));
  }
};

QTEST_GUILESS_MAIN(StyleActionsTest)
#include "tst_styleactions.moc"
