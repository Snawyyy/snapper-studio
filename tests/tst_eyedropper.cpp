#include <QBuffer>
#include <QImage>
#include <QTest>

#include "app/Theme.h"
#include "document/Document.h"
#include "edit/LookManager.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"
#include "edit/tools/EyedropperTool.h"
#include "render/ViewTransform.h"

using snapper::EyedropperTool;
using snapper::Id;

namespace {

struct Fixture {
  Fixture() {
    document.ResetToNew();
    view.SetViewSize(QSizeF(200, 200));
    view.Fit(QRectF(0, 0, 200, 200), 0);
    // A filled square from 20 to 80, red line, green fill.
    snapper::Shape square;
    square.id = document.NewId();
    square.layer = document.Layers().All().front().id;
    square.closed = true;
    square.look.has_fill = true;
    square.look.fill = Qt::green;
    square.look.line = Qt::red;
    for (const QPointF corner :
         {QPointF(20, 20), QPointF(80, 20), QPointF(80, 80), QPointF(20, 80)}) {
      square.points.push_back(
          {.id = document.NewId(), .pos = corner, .curve = 0.0F});
    }
    document.Shapes().Insert(square, 0);
    square_id = square.id;
  }

  snapper::ToolInput At(QPointF pos, Qt::KeyboardModifiers modifiers = {}) {
    return {.canvas = view.ToCanvas(pos),
            .screen = pos,
            .modifiers = modifiers,
            .view = &view};
  }

  snapper::Document document;
  snapper::UndoManager undo;
  snapper::SelectionManager selection{document};
  QString hint;
  snapper::LookManager looks{document, selection, undo, [](const QString&) {}};
  snapper::Theme theme;
  snapper::ViewTransform view;
  EyedropperTool tool{{.document = document,
                       .undo = undo,
                       .selection = selection,
                       .looks = looks,
                       .theme = theme,
                       .hint = [this](const QString& text) { hint = text; }}};
  Id square_id = snapper::kNoId;
};

}  // namespace

class EyedropperTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void ClickInsideAFillPicksItsColor() {
    Fixture f;
    f.tool.Press(f.At(QPointF(50, 50)));
    QVERIFY(f.looks.NewShapes().look.has_fill);
    QCOMPARE(f.looks.NewShapes().look.fill, QColor(Qt::green));
  }

  // New shapes take their line color from the default style.
  void LineColorFromAStyleSaysSo() {
    Fixture f;
    const EyedropperTool::Pick pick =
        f.tool.PickAt(f.At(QPointF(50, 20)))
            .value_or(
                EyedropperTool::Pick{.kind = EyedropperTool::Pick::Kind::kFill,
                                     .color = {},
                                     .shape = std::nullopt});
    QCOMPARE(pick.kind, EyedropperTool::Pick::Kind::kLine);
    f.tool.Press(f.At(QPointF(50, 20)));
    QVERIFY(f.hint.contains(QStringLiteral("Ink")));
  }

  void AltClickPicksTheWholeStyle() {
    Fixture f;
    f.tool.Press(f.At(QPointF(50, 50), Qt::AltModifier));
    const snapper::Shape* square = f.document.Shapes().Find(f.square_id);
    QCOMPARE(f.looks.NewShapes().look, square->look);
    QCOMPARE(f.looks.NewShapes().styles, square->styles);
  }

  void PicksIntoTheSelection() {
    Fixture f;
    snapper::Shape line;
    line.id = f.document.NewId();
    line.layer = f.document.Layers().All().front().id;
    for (int i = 0; i < 2; ++i) {
      line.points.push_back(
          {.id = f.document.NewId(), .pos = QPointF(150, 150 + i * 20)});
    }
    f.document.Shapes().Insert(line, 1);
    f.selection.SetPoints({line.points[0].id});
    f.tool.Press(f.At(QPointF(50, 50)));
    QCOMPARE(f.document.Shapes().Find(line.id)->look.fill, QColor(Qt::green));
    QVERIFY(f.undo.CanUndo());
  }

  void PicksAPicturePixel() {
    Fixture f;
    QImage picture(10, 10, QImage::Format_ARGB32);
    picture.fill(Qt::blue);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    picture.save(&buffer, "PNG");
    QString error;
    const QString name =
        f.document.Images().Add(bytes, &error).value_or(QString());
    f.document.Layers().Insert({.id = f.document.NewId(),
                                .type = snapper::LayerType::kImage,
                                .name = QStringLiteral("ref"),
                                .image = name,
                                .image_pos = QPointF(100, 100),
                                .image_scale = 2.0},
                               f.document.Layers().Count());
    const EyedropperTool::Pick pick =
        f.tool.PickAt(f.At(QPointF(115, 115))).value_or(EyedropperTool::Pick{});
    QCOMPARE(pick.kind, EyedropperTool::Pick::Kind::kPicture);
    QCOMPARE(pick.color, QColor(Qt::blue));
    QVERIFY(!f.tool.PickAt(f.At(QPointF(150, 30))).has_value());
  }
};

QTEST_MAIN(EyedropperTest)
#include "tst_eyedropper.moc"
