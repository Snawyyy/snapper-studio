#include <QTest>
#include <QUndoCommand>

#include "document/Document.h"
#include "edit/LayerCommands.h"
#include "edit/ShapeCommands.h"
#include "edit/UndoManager.h"

using snapper::Document;
using snapper::Id;
using snapper::MovePointsCommand;
using snapper::Shape;
using snapper::UndoManager;

namespace {

// A shape on the document's first layer with `count` points in a row.
Shape Row(Document& document, int count) {
  Shape shape;
  shape.id = document.NewId();
  shape.layer = document.Layers().All().front().id;
  for (int i = 0; i < count; ++i) {
    shape.points.push_back({.id = document.NewId(), .pos = QPointF(i, 0)});
  }
  return shape;
}

Id PointOf(const Shape& shape, int index) {
  return shape.points[static_cast<size_t>(index)].id;
}

}  // namespace

class UndoManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void AddShapeUndoesAndRedoes() {
    Document document;
    document.ResetToNew();
    UndoManager undo;
    undo.Push(snapper::MakeAddShape(document, Row(document, 3),
                                    QStringLiteral("Draw Line")));
    QCOMPARE(document.Shapes().Count(), 1);
    QCOMPARE(undo.UndoText(), QStringLiteral("Draw Line"));
    undo.Undo();
    QCOMPARE(document.Shapes().Count(), 0);
    undo.Redo();
    QCOMPARE(document.Shapes().Count(), 1);
  }

  void RemovingPointsKeepsShapeOrder() {
    Document document;
    document.ResetToNew();
    UndoManager undo;
    const Shape a = Row(document, 2);
    const Shape b = Row(document, 3);
    const Shape c = Row(document, 2);
    for (const Shape& shape : {a, b, c}) {
      undo.Push(snapper::MakeAddShape(document, shape, QString()));
    }
    // a loses everything, b keeps two points, c is untouched.
    undo.Push(snapper::MakeRemovePoints(
        document, {PointOf(a, 0), PointOf(a, 1), PointOf(b, 1)}));
    QCOMPARE(document.Shapes().Count(), 2);
    QCOMPARE(document.Shapes().IndexOf(b.id), 0);
    QCOMPARE(document.Shapes().IndexOf(c.id), 1);
    QCOMPARE(document.Shapes().Find(b.id)->points.size(), size_t{2});

    undo.Undo();
    QCOMPARE(document.Shapes().IndexOf(a.id), 0);
    QCOMPARE(document.Shapes().IndexOf(b.id), 1);
    QCOMPARE(document.Shapes().IndexOf(c.id), 2);
    QCOMPARE(document.Shapes().Find(b.id)->points.size(), size_t{3});
  }

  void RemovingUnknownPointsDoesNothing() {
    Document document;
    document.ResetToNew();
    QVERIFY(snapper::MakeRemovePoints(document, {12345}) == nullptr);
  }

  void OneDragIsOneStep() {
    Document document;
    document.ResetToNew();
    UndoManager undo;
    const Shape shape = Row(document, 2);
    undo.Push(snapper::MakeAddShape(document, shape, QString()));
    const Id point = PointOf(shape, 0);
    const int tag = MovePointsCommand::NewDragTag();
    for (int step = 1; step <= 3; ++step) {
      undo.Push(std::make_unique<MovePointsCommand>(
          document,
          std::vector<snapper::PointPosition>{{.point = point, .pos = {}}},
          std::vector<snapper::PointPosition>{
              {.point = point, .pos = QPointF(step, step)}},
          tag));
    }
    QCOMPARE(document.Shapes().PositionOf(point).value_or(QPointF()),
             QPointF(3, 3));
    undo.Undo();
    QCOMPARE(document.Shapes().PositionOf(point).value_or(QPointF(-1, -1)),
             QPointF(0, 0));
    QCOMPARE(undo.UndoText(), QString());
  }

  void DeletingALayerBringsItsShapesBack() {
    Document document;
    document.ResetToNew();
    UndoManager undo;
    const Id layer = document.Layers().All().front().id;
    undo.Push(snapper::MakeAddShape(document, Row(document, 2), QString()));
    undo.Push(std::make_unique<snapper::RemoveLayerCommand>(document, layer));
    QCOMPARE(document.Layers().Count(), 0);
    QCOMPARE(document.Shapes().Count(), 0);
    undo.Undo();
    QCOMPARE(document.Layers().Count(), 1);
    QCOMPARE(document.Shapes().Count(), 1);
  }
};

QTEST_GUILESS_MAIN(UndoManagerTest)
#include "tst_undomanager.moc"
