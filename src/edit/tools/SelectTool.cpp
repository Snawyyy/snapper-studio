#include "edit/tools/SelectTool.h"

#include <QLineF>
#include <QPainter>

#include <algorithm>
#include <cmath>

#include "app/Theme.h"
#include "document/Document.h"
#include "edit/HitTest.h"
#include "edit/SelectionCommands.h"
#include "edit/SelectionManager.h"
#include "edit/ShapeCommands.h"
#include "edit/UndoManager.h"
#include "edit/tools/LayerCheck.h"
#include "render/ShapeGeometry.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

// A press that moves less than this is a click.
constexpr double kDragStartPx = 3.0;
constexpr double kPointSizePx = 7.0;
constexpr double kNudge = 1.0;
constexpr double kBigNudge = 10.0;

QSet<Id> PointsOf(const Shape& shape) {
  QSet<Id> points;
  for (const Point& point : shape.points) {
    points.insert(point.id);
  }
  return points;
}

}  // namespace

SelectTool::SelectTool(ToolContext context) : Tool(std::move(context)) {}

ToolInfo SelectTool::Info() const {
  return {.id = QStringLiteral("select"),
          .title = QStringLiteral("Select"),
          .key = QKeySequence(Qt::Key_V),
          .hint = QStringLiteral(
              "Select: click or drag a box, Alt+drag for a lasso. Drag to "
              "move. Alt+click picks a layer.")};
}

void SelectTool::Press(const ToolInput& input) {
  press_screen_ = input.screen;
  press_canvas_ = input.canvas;
  press_modifiers_ = input.modifiers;
  press_target_ = TargetAt(input);
  area_.clear();

  if (press_target_.isEmpty()) {
    state_ = State::kPressedOnEmpty;
    return;
  }
  const QSet<Id>& selected = Context().selection.Points();
  if (input.modifiers.testFlag(Qt::ControlModifier)) {
    Select(QSet<Id>(selected).subtract(press_target_));
    state_ = State::kIdle;
    return;
  }
  if (input.modifiers.testFlag(Qt::ShiftModifier)) {
    Select(QSet<Id>(selected).unite(press_target_));
  } else if (!selected.contains(press_target_)) {
    Select(press_target_);
  }
  state_ = State::kPressedOnSelection;
}

void SelectTool::Drag(const ToolInput& input) {
  const bool is_past_threshold =
      QLineF(press_screen_, input.screen).length() >= kDragStartPx;
  switch (state_) {
    case State::kPressedOnSelection:
      if (is_past_threshold) {
        StartMove();
      }
      break;
    case State::kPressedOnEmpty:
      if (is_past_threshold) {
        const bool is_lasso = press_modifiers_.testFlag(Qt::AltModifier);
        state_ = is_lasso ? State::kLasso : State::kBox;
        area_ << press_screen_;
      }
      break;
    default:
      break;
  }

  if (state_ == State::kMoving) {
    QPointF delta = input.canvas - press_canvas_;
    // Shift keeps the move on one axis, whichever it is closer to.
    const bool is_locked = input.modifiers.testFlag(Qt::ShiftModifier);
    if (is_locked) {
      const bool is_sideways = std::abs(delta.x()) >= std::abs(delta.y());
      delta = is_sideways ? QPointF(delta.x(), 0) : QPointF(0, delta.y());
    }
    MoveBy(delta);
  } else if (state_ == State::kBox) {
    area_ = QPolygonF(QRectF(press_screen_, input.screen).normalized());
  } else if (state_ == State::kLasso) {
    area_ << input.screen;
  }
}

void SelectTool::Release(const ToolInput& input) {
  switch (state_) {
    case State::kPressedOnSelection: {
      // A plain click inside a bigger selection narrows it to what was
      // clicked.
      const bool is_plain = input.modifiers == Qt::NoModifier;
      if (is_plain) {
        Select(press_target_);
      }
      break;
    }
    case State::kPressedOnEmpty: {
      const bool is_layer_pick = press_modifiers_.testFlag(Qt::AltModifier);
      const bool is_adding = press_modifiers_.testAnyFlags(Qt::ShiftModifier |
                                                           Qt::ControlModifier);
      if (is_layer_pick) {
        const Id layer =
            HitTest::LayerAt(Context().document, *input.view, input.screen);
        if (Context().selection.SetActiveLayer(layer)) {
          Context().hint(
              QStringLiteral("Editing %1")
                  .arg(Context().document.Layers().Find(layer)->name));
        }
      } else if (!is_adding) {
        Select({});
      }
      break;
    }
    case State::kBox:
    case State::kLasso:
      FinishArea(input);
      break;
    default:
      break;
  }
  state_ = State::kIdle;
  area_.clear();
}

void SelectTool::DoubleClick(const ToolInput& input) {
  const Id shape_id =
      HitTest::ShapeAt(Context().document, Context().selection.ActiveLayer(),
                       *input.view, input.screen);
  const Shape* shape = Context().document.Shapes().Find(shape_id);
  if (shape != nullptr) {
    Select(PointsOf(*shape));
  }
}

// Escape mid-move puts the points back where they were.
void SelectTool::Cancel() {
  const bool has_moved = state_ == State::kMoving && !move_from_.empty();
  if (has_moved) {
    Context().undo.Undo();
  }
  state_ = State::kIdle;
  area_.clear();
}

bool SelectTool::KeyPress(int key, Qt::KeyboardModifiers modifiers) {
  const bool has_selection = !Context().selection.Points().isEmpty();
  if (!has_selection || state_ != State::kIdle) {
    return false;
  }
  const double step =
      modifiers.testFlag(Qt::ShiftModifier) ? kBigNudge : kNudge;
  QPointF delta;
  switch (key) {
    case Qt::Key_Left:
      delta = QPointF(-step, 0);
      break;
    case Qt::Key_Right:
      delta = QPointF(step, 0);
      break;
    case Qt::Key_Up:
      delta = QPointF(0, -step);
      break;
    case Qt::Key_Down:
      delta = QPointF(0, step);
      break;
    default:
      return false;
  }
  StartMove();
  MoveBy(delta);
  state_ = State::kIdle;
  return true;
}

QSet<Id> SelectTool::TargetAt(const ToolInput& input) const {
  const Document& document = Context().document;
  const Id layer = Context().selection.ActiveLayer();
  const Id point = HitTest::PointAt(document, layer, *input.view, input.screen);
  if (point != kNoId) {
    return {point};
  }
  const Shape* shape = document.Shapes().Find(
      HitTest::ShapeAt(document, layer, *input.view, input.screen));
  return shape == nullptr ? QSet<Id>() : PointsOf(*shape);
}

void SelectTool::Select(const QSet<Id>& points) {
  Context().undo.Push(MakeSelectPoints(Context().selection, points));
}

void SelectTool::StartMove() {
  if (!CanChangeActiveLayer(Context(), QStringLiteral("edit"))) {
    state_ = State::kIdle;
    return;
  }
  move_from_.clear();
  for (const Id point : Context().selection.Points()) {
    const std::optional<QPointF> pos =
        Context().document.Shapes().PositionOf(point);
    if (pos.has_value()) {
      move_from_.push_back({.point = point, .pos = *pos});
    }
  }
  move_tag_ = MovePointsCommand::NewDragTag();
  state_ = State::kMoving;
}

void SelectTool::MoveBy(const QPointF& canvas_delta) {
  if (move_from_.empty()) {
    return;
  }
  std::vector<PointPosition> to = move_from_;
  for (PointPosition& position : to) {
    position.pos += canvas_delta;
  }
  Context().undo.Push(std::make_unique<MovePointsCommand>(
      Context().document, move_from_, std::move(to), move_tag_));
}

void SelectTool::FinishArea(const ToolInput& input) {
  const QSet<Id> inside = HitTest::PointsInside(
      Context().document, Context().selection.ActiveLayer(), *input.view,
      area_);
  const QSet<Id>& selected = Context().selection.Points();
  if (press_modifiers_.testFlag(Qt::ShiftModifier)) {
    Select(QSet<Id>(selected).unite(inside));
  } else if (press_modifiers_.testFlag(Qt::ControlModifier)) {
    Select(QSet<Id>(selected).subtract(inside));
  } else {
    Select(inside);
  }
}

void SelectTool::DrawOverlay(QPainter& painter,
                             const ViewTransform& view) const {
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  DrawPoints(painter, view);
  const bool is_area = state_ == State::kBox || state_ == State::kLasso;
  if (is_area) {
    const Theme& theme = Context().theme;
    painter.setPen(QPen(theme.accent, 1.0));
    painter.setBrush(theme.accent_pill);
    painter.drawPolygon(area_);
  }
  painter.restore();
}

// Every point on the active layer, so you can see what there is to grab.
// Shapes with selected points get an accent outline.
void SelectTool::DrawPoints(QPainter& painter,
                            const ViewTransform& view) const {
  const Theme& theme = Context().theme;
  const SelectionManager& selection = Context().selection;
  const QTransform to_screen = view.ToScreenMatrix();
  const double half = kPointSizePx / 2;
  for (const Shape& shape : Context().document.Shapes().All()) {
    if (shape.layer != selection.ActiveLayer()) {
      continue;
    }
    const bool is_touched = std::ranges::any_of(
        shape.points,
        [&](const Point& point) { return selection.IsSelected(point.id); });
    if (is_touched) {
      painter.setPen(QPen(theme.accent, 1.0));
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(to_screen.map(ShapeGeometry::CenterLine(shape)));
    }
    for (const Point& point : shape.points) {
      const QPointF at = view.ToScreen(point.pos);
      const QRectF box(at - QPointF(half, half),
                       QSizeF(kPointSizePx, kPointSizePx));
      const bool is_selected = selection.IsSelected(point.id);
      painter.setPen(QPen(is_selected ? Qt::white : theme.window, 1.0));
      painter.setBrush(is_selected ? theme.accent : QColor(Qt::white));
      painter.drawRect(box);
    }
  }
}

}  // namespace snapper
