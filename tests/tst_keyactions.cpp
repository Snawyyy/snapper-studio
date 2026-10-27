#include <QTest>

#include "document/Document.h"
#include "edit/AnimationEditor.h"
#include "edit/KeyActions.h"
#include "edit/Playhead.h"
#include "edit/SelectionManager.h"
#include "edit/ShapeCommands.h"
#include "edit/UndoManager.h"
#include "eval/FrameEvaluator.h"

using snapper::ChannelProperty;
using snapper::Id;
using snapper::KeyRef;
using snapper::KeyRow;

namespace {

struct Fixture {
  Fixture() {
    document.ResetToNew();
    layer = document.Layers().All().front().id;
    selection.SetActiveLayer(layer);
    snapper::Shape line;
    line.id = document.NewId();
    line.layer = layer;
    for (const QPointF pos : {QPointF(0, 0), QPointF(100, 0)}) {
      line.points.push_back({.id = document.NewId(), .pos = pos});
    }
    document.Shapes().Insert(line, 0);
    point = line.points[1].id;
  }

  void MovePoint(const QPointF& to) {
    const QPointF from =
        frames.Current().PositionOf(point).value_or(QPointF(-1, -1));
    undo.Push(animation.MovePoints({{.point = point, .pos = from}},
                                   {{.point = point, .pos = to}},
                                   snapper::MovePointsCommand::NewDragTag()));
  }

  snapper::Document document;
  snapper::Playhead playhead{document};
  snapper::FrameEvaluator frames{document, playhead};
  snapper::AnimationEditor animation{document, playhead};
  snapper::UndoManager undo;
  snapper::SelectionManager selection{document};
  QString hint;
  snapper::KeyActions keys{
      document, selection, undo,
      playhead, frames,    [this](const QString& text) { hint = text; }};
  Id layer = snapper::kNoId;
  Id point = snapper::kNoId;
};

}  // namespace

class KeyActionsTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void MovingOnFrameZeroChangesTheSetup() {
    Fixture f;
    f.MovePoint(QPointF(100, 40));
    QCOMPARE(f.document.Shapes().PositionOf(f.point).value_or(QPointF(-1, -1)),
             QPointF(100, 40));
    QCOMPARE(f.document.Channels().Count(), 0);
  }

  void MovingOnALaterFrameSetsAKey() {
    Fixture f;
    f.playhead.SetFrame(10);
    f.MovePoint(QPointF(100, 40));
    QCOMPARE(f.document.Shapes().PositionOf(f.point).value_or(QPointF(-1, -1)),
             QPointF(100, 0));
    QCOMPARE(f.frames.Current().PositionOf(f.point).value_or(QPointF(-1, -1)),
             QPointF(100, 40));
    QCOMPARE(f.keys.FramesIn(KeyRow::kPoints), std::vector<int>{10});
    // Halfway there on frame 5, and back to the setup on frame 0.
    QVERIFY(f.frames.At(5).PositionOf(f.point).value_or(QPointF()).y() > 0);
    QCOMPARE(f.frames.At(0).PositionOf(f.point).value_or(QPointF(-1, -1)),
             QPointF(100, 0));
    f.undo.Undo();
    QCOMPARE(f.document.Channels().Count(), 0);
  }

  void PlacingALayerKeysOnlyWhatChanged() {
    Fixture f;
    f.playhead.SetFrame(8);
    snapper::LayerTransform moved;
    moved.position = QPointF(30, 0);
    f.undo.Push(f.animation.PlaceLayer(f.layer, moved, std::nullopt, 1));
    QCOMPARE(f.keys.FramesIn(KeyRow::kMove), std::vector<int>{8});
    QVERIFY(f.keys.FramesIn(KeyRow::kRotate).empty());
    QCOMPARE(f.frames.Current().TransformOf(f.layer).position, QPointF(30, 0));
  }

  void AddKeysWithoutPointsKeysTheLayer() {
    Fixture f;
    f.keys.AddKeys();
    QVERIFY(f.hint.contains(QStringLiteral("setup")));
    f.playhead.SetFrame(4);
    f.keys.AddKeys();
    QCOMPARE(f.keys.FramesIn(KeyRow::kMove), std::vector<int>{4});
    QCOMPARE(f.keys.FramesIn(KeyRow::kScale), std::vector<int>{4});
    QVERIFY(f.keys.FramesIn(KeyRow::kPoints).empty());
  }

  void SelectedKeysMoveAndCopy() {
    Fixture f;
    f.playhead.SetFrame(10);
    f.MovePoint(QPointF(100, 40));
    f.keys.Select({{.row = KeyRow::kAll, .frame = 10}});
    f.keys.MoveSelected(5, false);
    QCOMPARE(f.keys.FramesIn(KeyRow::kPoints), std::vector<int>{15});
    QCOMPARE(f.keys.Selected(), (std::vector<KeyRef>{{KeyRow::kAll, 15}}));
    f.keys.MoveSelected(-3, true);
    QCOMPARE(f.keys.FramesIn(KeyRow::kPoints), (std::vector<int>{12, 15}));
  }

  void CurvesAndDeletingKeys() {
    Fixture f;
    f.playhead.SetFrame(10);
    f.MovePoint(QPointF(100, 40));
    f.keys.Select({{.row = KeyRow::kPoints, .frame = 10}});
    f.keys.SetCurve(snapper::Curve::kStep);
    QCOMPARE(f.keys.SelectedCurve(), snapper::Curve::kStep);
    f.keys.DeleteKeys();
    QVERIFY(f.keys.FramesIn(KeyRow::kAll).empty());
    QVERIFY(f.keys.Selected().empty());
  }

  void PlayheadStaysInsideTheFrames() {
    Fixture f;
    f.playhead.SetFrame(100000);
    QCOMPARE(f.playhead.Frame(), f.playhead.LastFrame());
    f.playhead.SetFrame(-5);
    QCOMPARE(f.playhead.Frame(), 0);
  }
};

QTEST_GUILESS_MAIN(KeyActionsTest)
#include "tst_keyactions.moc"
