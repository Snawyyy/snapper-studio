#include <QImage>
#include <QTemporaryDir>
#include <QTest>

#include "document/Document.h"
#include "edit/BrushActions.h"
#include "edit/LookManager.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"

using snapper::BrushActions;
using snapper::Id;

namespace {

struct Fixture {
  Fixture() { document.ResetToNew(); }

  snapper::Document document;
  snapper::UndoManager undo;
  snapper::SelectionManager selection{document};
  QString last_hint;
  snapper::LookManager looks{document, selection, undo, [](const QString&) {}};
  BrushActions actions{document, looks, undo,
                       [this](const QString& text) { last_hint = text; }};
};

QString SavePicture(const QTemporaryDir& dir, int side) {
  QImage image(side, side, QImage::Format_ARGB32);
  image.fill(Qt::black);
  const QString path = dir.filePath(QStringLiteral("dab.png"));
  image.save(path);
  return path;
}

}  // namespace

class BrushActionsTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void ImportAddsABrushNamedAfterTheFile() {
    Fixture f;
    const QTemporaryDir dir;
    const int before = f.document.Brushes().Count();
    QString why_not;
    const Id brush = f.actions.ImportBrush(SavePicture(dir, 16), &why_not);
    QVERIFY2(brush != snapper::kNoId, qPrintable(why_not));
    QCOMPARE(f.document.Brushes().Count(), before + 1);
    QCOMPARE(f.document.Brushes().Find(brush)->name, QStringLiteral("dab"));
    f.undo.Undo();
    QCOMPARE(f.document.Brushes().Count(), before);
  }

  void ImportRefusesBigPictures() {
    Fixture f;
    const QTemporaryDir dir;
    QString why_not;
    const Id brush = f.actions.ImportBrush(
        SavePicture(dir, BrushActions::kMaxPictureSide + 1), &why_not);
    QCOMPARE(brush, snapper::kNoId);
    QVERIFY(!why_not.isEmpty());
  }

  // The default style sets the brush, so the new look can't take one.
  void UseSaysWhenAStyleSetsTheBrush() {
    Fixture f;
    const Id pencil = f.document.Brushes().All().front().id;
    f.actions.UseBrush(pencil);
    QCOMPARE(f.looks.NewShapes().look.brush, snapper::kNoId);
    QVERIFY(f.last_hint.contains(QStringLiteral("Ink")));
    f.looks.Change(QString(),
                   [](snapper::ShapeLookState& state) { state.styles = {}; });
    f.actions.UseBrush(pencil);
    QCOMPARE(f.looks.NewShapes().look.brush, pencil);
  }

  void EditKeepsThePicture() {
    Fixture f;
    const snapper::Brush pencil = f.document.Brushes().All().front();
    f.actions.EditBrush(pencil.id, [](snapper::Brush& brush) {
      brush.image = QStringLiteral("elsewhere");
      brush.spacing = 1.0F;
    });
    QCOMPARE(f.document.Brushes().Find(pencil.id)->image, pencil.image);
    QCOMPARE(f.document.Brushes().Find(pencil.id)->spacing, 1.0F);
  }
};

QTEST_MAIN(BrushActionsTest)
#include "tst_brushactions.moc"
