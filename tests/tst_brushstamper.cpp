#include <QElapsedTimer>
#include <QImage>
#include <QPainter>
#include <QTest>

#include "document/Document.h"
#include "render/BrushStamper.h"
#include "render/Renderer.h"

using snapper::Brush;
using snapper::BrushStamp;
using snapper::BrushStamper;
using snapper::Shape;

namespace {

Shape Straight(QPointF from, QPointF to) {
  Shape shape;
  shape.id = 1;
  shape.points = {{.id = 2, .pos = from, .curve = 0.0F},
                  {.id = 3, .pos = to, .curve = 0.0F}};
  return shape;
}

}  // namespace

class BrushStamperTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void SpacingSetsTheGap() {
    const Brush brush{.spacing = 0.5F, .align = false};
    const auto stamps = BrushStamper::Plan(
        Straight(QPointF(0, 0), QPointF(100, 0)), 10.0, brush, 0);
    // A stamp every 5 px from 0 to 100.
    QCOMPARE(stamps.size(), size_t{21});
    QCOMPARE(stamps[1].pos.x(), 5.0);
    QCOMPARE(stamps.back().size, 10.0);
  }

  void AlignFollowsTheLine() {
    const Brush brush{.spacing = 1.0F, .align = true};
    const auto down = BrushStamper::Plan(
        Straight(QPointF(0, 0), QPointF(0, 50)), 5.0, brush, 0);
    QVERIFY(!down.empty());
    QCOMPARE(down[2].angle_degrees, 90.0);
  }

  // Jitter repeats for a shape until a boil frame changes it.
  void JitterRepeatsUntilItBoils() {
    const Brush brush{.spacing = 0.3F,
                      .angle_jitter = 90.0F,
                      .size_jitter = 0.5F,
                      .boil_every = 2};
    const Shape line = Straight(QPointF(0, 0), QPointF(80, 30));
    const auto angles = [&](int frame) {
      std::vector<double> out;
      for (const BrushStamp& stamp :
           BrushStamper::Plan(line, 6.0, brush, frame)) {
        out.push_back(stamp.angle_degrees);
      }
      return out;
    };
    QCOMPARE(angles(0), angles(1));
    QVERIFY(angles(0) != angles(2));
  }

  void StopsAtTheLimit() {
    const Brush brush{.spacing = Brush::kMinSpacing};
    const auto stamps = BrushStamper::Plan(
        Straight(QPointF(0, 0), QPointF(100000, 0)), 1.0, brush, 0);
    QCOMPARE(static_cast<int>(stamps.size()), BrushStamper::kMaxStampsPerShape);
  }

  // A picture without transparency paints with its dark parts.
  void TintCountsDarkWithoutAlpha() {
    QImage picture(2, 1, QImage::Format_RGB32);
    picture.setPixel(0, 0, qRgb(0, 0, 0));
    picture.setPixel(1, 0, qRgb(255, 255, 255));
    const QImage tinted = BrushStamper::Tint(picture, Qt::red, 2);
    QCOMPARE(qAlpha(tinted.pixel(0, 0)), 255);
    QCOMPARE(qRed(tinted.pixel(0, 0)), 255);
    QCOMPARE(qAlpha(tinted.pixel(1, 0)), 0);
  }

  // A brushed line paints near its middle, in its color.
  void RendererStampsBrushedLines() {
    snapper::Document document;
    document.ResetToNew();
    Shape line = Straight(QPointF(5, 20), QPointF(75, 20));
    line.look.line = Qt::blue;
    line.look.line_width = 12.0F;
    line.look.brush = document.Brushes().All()[1].id;  // ink
    QImage canvas(80, 40, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::white);
    {
      QPainter painter(&canvas);
      painter.setRenderHint(QPainter::Antialiasing);
      snapper::Renderer::DrawShape(painter, document, line);
    }
    const QColor middle = canvas.pixelColor(40, 20);
    QVERIFY(middle.blue() > 200);
    QVERIFY(middle.red() < 60);
    QCOMPARE(canvas.pixelColor(40, 2), QColor(Qt::white));
  }

  // Not a pass or fail, a number to watch: a page of brushed lines.
  void TimeAPageOfBrushedLines() {
    snapper::Document document;
    document.ResetToNew();
    const snapper::Id pencil = document.Brushes().All()[0].id;
    std::vector<Shape> lines;
    for (int i = 0; i < 100; ++i) {
      Shape line = Straight(QPointF(10, i * 10), QPointF(1500, i * 10 + 40));
      line.id = 10 + i;
      line.look.brush = pencil;
      line.look.line_width = 4.0F;
      lines.push_back(line);
    }
    QImage canvas(1600, 1100, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing);
    QElapsedTimer timer;
    timer.start();
    for (const Shape& line : lines) {
      snapper::Renderer::DrawShape(painter, document, line);
    }
    qInfo("100 pencil lines of 1500 px: %lld ms", timer.elapsed());
  }
};

QTEST_MAIN(BrushStamperTest)
#include "tst_brushstamper.moc"
