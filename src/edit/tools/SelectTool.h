#pragma once

#include <QPolygonF>
#include <QSet>

#include <cstdint>
#include <vector>

#include "document/ShapeManager.h"
#include "edit/Tool.h"

namespace snapper {

// Selects and moves points on the active layer. Click a point or a shape
// to select it, Shift+click adds, Ctrl+click removes. Drag on empty canvas
// for a box, Alt+drag for a lasso, and drag a selection to move it.
// Alt+click picks the layer under the pointer.
class SelectTool : public Tool {
 public:
  explicit SelectTool(ToolContext context);

  ToolInfo Info() const override;
  void Press(const ToolInput& input) override;
  void Drag(const ToolInput& input) override;
  void Release(const ToolInput& input) override;
  void DoubleClick(const ToolInput& input) override;
  void Cancel() override;
  bool KeyPress(int key, Qt::KeyboardModifiers modifiers) override;
  void DrawOverlay(QPainter& painter, const ViewTransform& view) const override;
  bool IsBusy() const override { return state_ != State::kIdle; }

 private:
  enum class State : std::uint8_t {
    kIdle,
    kPressedOnSelection,  // may become a move
    kMoving,
    kPressedOnEmpty,  // may become a box or a lasso
    kBox,
    kLasso,
  };

  // What a click at this spot would select: one point, a whole shape, or
  // nothing.
  QSet<Id> TargetAt(const ToolInput& input) const;
  void Select(const QSet<Id>& points);
  void StartMove();
  void MoveBy(const QPointF& canvas_delta);
  void FinishArea(const ToolInput& input);
  void DrawPoints(QPainter& painter, const ViewTransform& view) const;

  State state_ = State::kIdle;
  QPointF press_screen_;
  QPointF press_canvas_;
  Qt::KeyboardModifiers press_modifiers_;
  QSet<Id> press_target_;
  QPolygonF area_;
  std::vector<PointPosition> move_from_;
  int move_tag_ = 0;
};

}  // namespace snapper
