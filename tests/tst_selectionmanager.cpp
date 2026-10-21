#include <QTest>

#include "app/CommandRegistry.h"
#include "document/Document.h"
#include "edit/SelectionCommands.h"
#include "edit/SelectionManager.h"
#include "edit/ShapeCommands.h"
#include "edit/UndoManager.h"

using snapper::Id;

namespace {

struct Fixture {
  Fixture() {
    document.ResetToNew();
    selection.RegisterCommands(commands, undo);
    snapper::Shape shape;
    shape.id = document.NewId();
    shape.layer = selection.ActiveLayer();
    for (int i = 0; i < 3; ++i) {
      const Id id = document.NewId();
      points.insert(id);
      shape.points.push_back({.id = id, .pos = QPointF(i, i)});
    }
    undo.Push(snapper::MakeAddShape(document, shape, QString()));
  }

  snapper::Document document;
  snapper::UndoManager undo;
  snapper::SelectionManager selection{document};
  snapper::CommandRegistry commands;
  QSet<Id> points;
};

}  // namespace

class SelectionManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void SelectingIsUndone() {
    Fixture f;
    f.commands.Run(QStringLiteral("select.all"));
    QCOMPARE(f.selection.Points(), f.points);
    f.undo.Undo();
    QVERIFY(f.selection.Points().isEmpty());
  }

  void InvertFlipsTheSelection() {
    Fixture f;
    const Id one = *f.points.cbegin();
    f.undo.Push(snapper::MakeSelectPoints(f.selection, {one}));
    f.commands.Run(QStringLiteral("select.invert"));
    QCOMPARE(f.selection.Points(), QSet<Id>(f.points).subtract({one}));
  }

  void UndoingDeleteSelectsThePointsAgain() {
    Fixture f;
    f.commands.Run(QStringLiteral("select.all"));
    f.commands.Run(QStringLiteral("edit.delete"));
    QCOMPARE(f.document.Shapes().Count(), 0);
    QVERIFY(f.selection.Points().isEmpty());
    f.undo.Undo();
    QCOMPARE(f.document.Shapes().Count(), 1);
    QCOMPARE(f.selection.Points(), f.points);
  }

  void BoundsCoverTheSelection() {
    Fixture f;
    QVERIFY(!f.selection.Bounds().has_value());
    f.commands.Run(QStringLiteral("select.all"));
    QCOMPARE(f.selection.Bounds().value_or(QRectF()), QRectF(0, 0, 2, 2));
  }

  void SwitchingLayersDropsTheSelection() {
    Fixture f;
    f.commands.Run(QStringLiteral("select.all"));
    const snapper::Layer other{.id = f.document.NewId(),
                               .name = QStringLiteral("Other")};
    f.document.Layers().Insert(other, 1);
    QVERIFY(f.selection.SetActiveLayer(other.id));
    QVERIFY(f.selection.Points().isEmpty());
  }
};

QTEST_GUILESS_MAIN(SelectionManagerTest)
#include "tst_selectionmanager.moc"
