#include <QTest>

#include "TestStyles.h"
#include "document/Document.h"
#include "edit/ShapeCommands.h"
#include "edit/StyleCommands.h"
#include "edit/UndoManager.h"

using snapper::Document;
using snapper::Id;
using snapper::Shape;
using snapper::Style;
using snapper::UndoManager;

namespace {

Id AddLine(Document& document, snapper::StylePair styles, Id brush) {
  Shape shape;
  shape.id = document.NewId();
  shape.layer = document.Layers().All().front().id;
  shape.styles = styles;
  shape.look.brush = brush;
  for (int i = 0; i < 2; ++i) {
    shape.points.push_back({.id = document.NewId(), .pos = QPointF(i, 0)});
  }
  document.Shapes().Insert(shape, document.Shapes().Count());
  return shape.id;
}

}  // namespace

class StyleCommandsTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  // A drag of a style value is one undo step.
  void StyleEditsOfOneDragMerge() {
    Document document;
    document.ResetToNew();
    UndoManager undo;
    const Style original =
        *document.Styles().Find(snapper::test::AddInkStyle(document));
    const int tag = snapper::MovePointsCommand::NewDragTag();
    Style edited = original;
    for (int width = 4; width <= 6; ++width) {
      const Style before = *document.Styles().Find(original.id);
      edited.look.line_width = static_cast<float>(width);
      undo.Push(std::make_unique<snapper::SetStyleCommand>(document, before,
                                                           edited, tag));
    }
    QCOMPARE(document.Styles().Find(original.id)->look.line_width, 6.0F);
    QCOMPARE(undo.Stack()->count(), 1);
    undo.Undo();
    QCOMPARE(*document.Styles().Find(original.id), original);
  }

  // Dragging away and back again leaves no step.
  void DragBackToTheStartLeavesNoStep() {
    Document document;
    document.ResetToNew();
    UndoManager undo;
    const Style original =
        *document.Styles().Find(snapper::test::AddInkStyle(document));
    const int tag = snapper::MovePointsCommand::NewDragTag();
    Style edited = original;
    edited.look.line = Qt::red;
    undo.Push(std::make_unique<snapper::SetStyleCommand>(document, original,
                                                         edited, tag));
    undo.Push(std::make_unique<snapper::SetStyleCommand>(document, edited,
                                                         original, tag));
    QCOMPARE(undo.Stack()->count(), 0);
    QCOMPARE(*document.Styles().Find(original.id), original);
  }

  void DeletingAStyleFreesItsShapes() {
    Document document;
    document.ResetToNew();
    UndoManager undo;
    const Id style = snapper::test::AddInkStyle(document);
    const Id shape = AddLine(document, {style, snapper::kNoId}, snapper::kNoId);
    undo.Push(std::make_unique<snapper::RemoveStyleCommand>(document, style));
    QCOMPARE(document.Styles().Count(), 0);
    QCOMPARE(document.Shapes().Find(shape)->styles[0], snapper::kNoId);
    undo.Undo();
    QCOMPARE(document.Styles().Count(), 1);
    QCOMPARE(document.Shapes().Find(shape)->styles[0], style);
  }

  void DeletingABrushLeavesPlainLines() {
    Document document;
    document.ResetToNew();
    UndoManager undo;
    const Id brush = document.Brushes().All().front().id;
    Style style = *document.Styles().Find(snapper::test::AddInkStyle(document));
    style.look.brush = brush;
    document.Styles().Set(style);
    const Id shape = AddLine(document, {}, brush);
    undo.Push(std::make_unique<snapper::RemoveBrushCommand>(document, brush));
    QVERIFY(document.Brushes().Find(brush) == nullptr);
    QCOMPARE(document.Shapes().Find(shape)->look.brush, snapper::kNoId);
    QCOMPARE(document.Styles().Find(style.id)->look.brush, snapper::kNoId);
    undo.Undo();
    QVERIFY(document.Brushes().Find(brush) != nullptr);
    QCOMPARE(document.Shapes().Find(shape)->look.brush, brush);
    QCOMPARE(document.Styles().Find(style.id)->look.brush, brush);
  }

  void ShapeLooksSetAndUndo() {
    Document document;
    document.ResetToNew();
    UndoManager undo;
    const Id shape = AddLine(document, {}, snapper::kNoId);
    const auto before = snapper::LookStatesOf(document.Shapes(), {shape});
    auto after = before;
    after[0].look.line = Qt::red;
    undo.Push(std::make_unique<snapper::SetShapeLooksCommand>(
        document, QStringLiteral("Line Color"), before, after, 0));
    QCOMPARE(document.Shapes().Find(shape)->look.line, QColor(Qt::red));
    undo.Undo();
    QCOMPARE(document.Shapes().Find(shape)->look.line, QColor(Qt::black));
  }
};

QTEST_GUILESS_MAIN(StyleCommandsTest)
#include "tst_stylecommands.moc"
