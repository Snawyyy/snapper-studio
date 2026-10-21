#pragma once

#include <cstdint>

#include "document/ShapeManager.h"
#include "edit/Tool.h"

namespace snapper {

// Drags out a rectangle or an ellipse from corner to corner, in the look
// for new shapes. Shift makes it a square or circle, Alt grows it from the
// middle.
class ShapeTool : public Tool {
 public:
  enum class Kind : std::uint8_t { kRectangle, kEllipse };

  ShapeTool(ToolContext context, Kind kind);

  ToolInfo Info() const override;
  void Press(const ToolInput& input) override;
  void Drag(const ToolInput& input) override;
  void Release(const ToolInput& input) override;
  void Cancel() override { is_dragging_ = false; }
  void DrawOverlay(QPainter& painter, const ViewTransform& view) const override;
  bool IsBusy() const override { return is_dragging_; }

 private:
  QRectF DraggedRect() const;
  // The shape for a rect, with ids only when `with_ids` is set.
  Shape Build(const QRectF& rect, bool with_ids) const;

  Kind kind_;
  bool is_dragging_ = false;
  QPointF start_;
  QPointF end_;
  Qt::KeyboardModifiers modifiers_;
};

}  // namespace snapper
