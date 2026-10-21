#include <QSignalSpy>
#include <QTest>

#include "document/ShapeManager.h"

using snapper::Id;
using snapper::Shape;
using snapper::ShapeManager;

namespace {

// A line on layer 1 with points first_point and first_point + 1.
Shape Line(Id id, Id first_point) {
  Shape shape;
  shape.id = id;
  shape.layer = 1;
  shape.points.push_back({.id = first_point, .pos = QPointF(0, 0)});
  shape.points.push_back({.id = first_point + 1, .pos = QPointF(10, 0)});
  return shape;
}

}  // namespace

class ShapeManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void FindsPointsByIdAfterInsert() {
    ShapeManager shapes;
    QVERIFY(shapes.Insert(Line(1, 10), 0));
    QVERIFY(shapes.Insert(Line(2, 20), 0));
    QCOMPARE(shapes.IndexOf(2), 0);
    QCOMPARE(shapes.ShapeOfPoint(11), 1);
    QCOMPARE(shapes.ShapeOfPoint(20), 2);
    QCOMPARE(shapes.PositionOf(11).value_or(QPointF(-1, -1)), QPointF(10, 0));
    QCOMPARE(shapes.PointCount(), 4);
  }

  void RejectsRepeatedPointIds() {
    ShapeManager shapes;
    QVERIFY(shapes.Insert(Line(1, 10), 0));
    QVERIFY(!shapes.Insert(Line(2, 11), 0));
    QCOMPARE(shapes.Count(), 1);
  }

  void RejectsASinglePoint() {
    ShapeManager shapes;
    Shape shape = Line(1, 10);
    shape.points.pop_back();
    QVERIFY(!shapes.Insert(shape, 0));
  }

  void MovesPointsAndSkipsUnknownOnes() {
    ShapeManager shapes;
    shapes.Insert(Line(1, 10), 0);
    const QSignalSpy spy(&shapes, &ShapeManager::LayerContentChanged);
    const int moved = shapes.SetPositions(
        {{.point = 10, .pos = QPointF(5, 5)}, {.point = 99, .pos = {}}});
    QCOMPARE(moved, 1);
    QCOMPARE(shapes.PositionOf(10).value_or(QPointF(-1, -1)), QPointF(5, 5));
    QCOMPARE(spy.count(), 1);
  }

  void TakeForgetsItsPoints() {
    ShapeManager shapes;
    shapes.Insert(Line(1, 10), 0);
    shapes.Insert(Line(2, 20), 1);
    const snapper::RemovedShape removed =
        shapes.Take(1).value_or(snapper::RemovedShape{.index = -1});
    QCOMPARE(removed.index, 0);
    QCOMPARE(shapes.ShapeOfPoint(10), snapper::kNoId);
    QCOMPARE(shapes.IndexOf(2), 0);
  }
};

QTEST_GUILESS_MAIN(ShapeManagerTest)
#include "tst_shapemanager.moc"
