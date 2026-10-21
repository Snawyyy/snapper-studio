#include <QTest>

#include "document/Document.h"
#include "edit/LayerActions.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"

using snapper::Id;

namespace {

struct Fixture {
  Fixture() { document.ResetToNew(); }

  Id LayerAt(int index) const {
    return document.Layers().All()[static_cast<size_t>(index)].id;
  }

  snapper::Document document;
  snapper::UndoManager undo;
  snapper::SelectionManager selection{document};
  QString last_hint;
  snapper::LayerActions actions{
      document, selection, undo,
      [this](const QString& text) { last_hint = text; }};
};

}  // namespace

class LayerActionsTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void NewLayerGoesAboveTheActiveOneAndBecomesActive() {
    Fixture f;
    const Id first = f.LayerAt(0);
    f.actions.AddLayer();
    f.selection.SetActiveLayer(first);
    f.actions.AddLayer();
    QCOMPARE(f.document.Layers().Count(), 3);
    QCOMPARE(f.selection.ActiveLayer(), f.LayerAt(1));
    QCOMPARE(f.document.Layers().Find(f.LayerAt(1))->name,
             QStringLiteral("Layer 3"));
  }

  void TheLastLayerStays() {
    Fixture f;
    QString why_not;
    QVERIFY(!f.actions.DeleteLayer(f.LayerAt(0), &why_not));
    QVERIFY(!why_not.isEmpty());
    QCOMPARE(f.document.Layers().Count(), 1);
  }

  void MovingStopsAtTheEnds() {
    Fixture f;
    const Id bottom = f.LayerAt(0);
    f.actions.AddLayer();
    f.actions.MoveLayer(bottom, 1);
    QCOMPARE(f.document.Layers().IndexOf(bottom), 1);
    f.actions.MoveLayer(bottom, 1);
    QCOMPARE(f.document.Layers().IndexOf(bottom), 1);
    f.undo.Undo();
    QCOMPARE(f.document.Layers().IndexOf(bottom), 0);
  }

  void BlankNamesAreIgnored() {
    Fixture f;
    f.actions.RenameLayer(f.LayerAt(0), QStringLiteral("   "));
    QVERIFY(!f.undo.CanUndo());
    f.actions.RenameLayer(f.LayerAt(0), QStringLiteral(" Head "));
    QCOMPARE(f.document.Layers().Find(f.LayerAt(0))->name,
             QStringLiteral("Head"));
  }
};

QTEST_GUILESS_MAIN(LayerActionsTest)
#include "tst_layeractions.moc"
