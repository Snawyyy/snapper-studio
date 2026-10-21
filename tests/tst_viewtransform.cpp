#include <QTest>

#include "render/ViewTransform.h"

using snapper::ViewTransform;

namespace {

bool Near(const QPointF& a, const QPointF& b) {
  return QLineF(a, b).length() < 1e-6;
}

ViewTransform View() {
  ViewTransform view;
  view.SetViewSize(QSizeF(800, 600));
  return view;
}

}  // namespace

class ViewTransformTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void ZoomKeepsThePointUnderThePointer() {
    ViewTransform view = View();
    const QPointF pointer(100, 450);
    const QPointF before = view.ToCanvas(pointer);
    view.ZoomAt(pointer, 3.0);
    QVERIFY(Near(view.ToCanvas(pointer), before));
    QCOMPARE(view.Zoom(), 3.0);
  }

  void RotateKeepsThePointInPlace() {
    ViewTransform view = View();
    view.ZoomAt(QPointF(10, 10), 2.0);
    const QPointF pivot(600, 100);
    const QPointF before = view.ToCanvas(pivot);
    view.RotateAt(pivot, 37.0);
    QVERIFY(Near(view.ToCanvas(pivot), before));
  }

  void PanMovesContentWithThePointer() {
    ViewTransform view = View();
    const QPointF canvas_point = view.ToCanvas(QPointF(200, 200));
    view.PanBy(QPointF(30, -20));
    QVERIFY(Near(view.ToScreen(canvas_point), QPointF(230, 180)));
  }

  void ZoomStaysInItsLimits() {
    ViewTransform view = View();
    view.ZoomAt(QPointF(), 1e9);
    QCOMPARE(view.Zoom(), ViewTransform::kMaxZoom);
    view.ZoomAt(QPointF(), 1e-12);
    QCOMPARE(view.Zoom(), ViewTransform::kMinZoom);
  }

  void FitShowsTheWholeRect() {
    ViewTransform view = View();
    view.RotateAt(QPointF(), 45);
    view.Fit(QRectF(0, 0, 1920, 1080), 40);
    QCOMPARE(view.Angle(), 0.0);
    const QPointF top_left = view.ToScreen(QPointF(0, 0));
    const QPointF bottom_right = view.ToScreen(QPointF(1920, 1080));
    QVERIFY(top_left.x() >= 40 - 1e-6);
    QVERIFY(bottom_right.x() <= 760 + 1e-6);
    QVERIFY(top_left.y() >= 0);
    QVERIFY(bottom_right.y() <= 600);
  }
};

QTEST_GUILESS_MAIN(ViewTransformTest)
#include "tst_viewtransform.moc"
