#include <QTest>

#include "TestStyles.h"
#include "document/Document.h"
#include "edit/LookManager.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"

using snapper::Id;
using snapper::LookManager;
using snapper::ShapeLookState;

namespace {

struct Fixture {
  Fixture() { document.ResetToNew(); }

  Id AddLine() {
    snapper::Shape shape;
    shape.id = document.NewId();
    shape.layer = document.Layers().All().front().id;
    for (int i = 0; i < 3; ++i) {
      shape.points.push_back({.id = document.NewId(), .pos = QPointF(i, 0)});
    }
    document.Shapes().Insert(shape, document.Shapes().Count());
    return shape.id;
  }

  Id UseInkForNewShapes() {
    const Id ink = snapper::test::AddInkStyle(document);
    looks.Change(QString(), [ink](snapper::ShapeLookState& state) {
      state.styles[0] = ink;
    });
    return ink;
  }

  snapper::Document document;
  snapper::UndoManager undo;
  snapper::SelectionManager selection{document};
  QString last_hint;
  LookManager looks{document, selection, undo,
                    [this](const QString& text) { last_hint = text; }};
};

void MakeRed(ShapeLookState& state) { state.look.line = Qt::red; }

}  // namespace

class LookManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  // Like Moho, a new project has no styles for new shapes to start on.
  void NewShapesStartWithoutStyles() {
    const Fixture f;
    QCOMPARE(f.looks.NewShapes().styles,
             (snapper::StylePair{snapper::kNoId, snapper::kNoId}));
  }

  // Nothing selected: the change goes to new shapes, outside undo.
  void ChangeWithoutSelectionSetsNewShapes() {
    Fixture f;
    const Id line = f.AddLine();
    f.looks.Change(QStringLiteral("Line Color"), MakeRed);
    QCOMPARE(f.looks.NewShapes().look.line, QColor(Qt::red));
    QCOMPARE(f.document.Shapes().Find(line)->look.line, QColor(Qt::black));
    QVERIFY(!f.undo.CanUndo());
  }

  // One selected point is enough to count its shape.
  void ChangeWithSelectionGoesThroughUndo() {
    Fixture f;
    const Id first = f.AddLine();
    const Id second = f.AddLine();
    f.selection.SetPoints({f.document.Shapes().Find(second)->points[1].id});
    QCOMPARE(f.looks.SelectedShapes(), std::vector<Id>{second});
    f.looks.Change(QStringLiteral("Line Color"), MakeRed);
    QCOMPARE(f.document.Shapes().Find(second)->look.line, QColor(Qt::red));
    QCOMPARE(f.document.Shapes().Find(first)->look.line, QColor(Qt::black));
    QCOMPARE(f.looks.Shown().shape, second);
    f.undo.Undo();
    QCOMPARE(f.document.Shapes().Find(second)->look.line, QColor(Qt::black));
  }

  void WidthSetByAStyleSaysSo() {
    Fixture f;
    snapper::Style style = *f.document.Styles().Find(f.UseInkForNewShapes());
    style.parts.width = true;
    f.document.Styles().Set(style);
    QCOMPARE(f.looks.StyleSetting(snapper::LookPart::kWidth), style.name);
    const float before = f.looks.NewShapes().look.line_width;
    f.looks.StepWidth(true);
    QCOMPARE(f.looks.NewShapes().look.line_width, before);
    QVERIFY(f.last_hint.contains(style.name));
  }

  void WidthStepsStayInRange() {
    Fixture f;
    for (int i = 0; i < 100; ++i) {
      f.looks.StepWidth(true);
    }
    QCOMPARE(f.looks.NewShapes().look.line_width, LookManager::kMaxLineWidth);
  }

  void DeletedStyleLeavesNewShapes() {
    Fixture f;
    const Id style = f.UseInkForNewShapes();
    f.document.Styles().Take(style);
    QCOMPARE(f.looks.NewShapes().styles[0], snapper::kNoId);
  }
};

QTEST_GUILESS_MAIN(LookManagerTest)
#include "tst_lookmanager.moc"
