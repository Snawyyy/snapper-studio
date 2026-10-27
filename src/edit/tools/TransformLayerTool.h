#pragma once

#include <QPointF>
#include <QRectF>

#include <array>
#include <cstdint>
#include <optional>

#include "document/LayerManager.h"
#include "edit/Tool.h"

namespace snapper {

// Moves, turns and scales the active layer, Figma style, with a box round
// its art: drag inside to move, a corner to scale, just outside a corner to
// turn. Shift keeps a move on one axis, scales evenly, and snaps turns to
// 15 degrees. On frame 0 it moves the setup pose, on other frames it sets
// keys.
class TransformLayerTool : public Tool {
 public:
  explicit TransformLayerTool(ToolContext context);

  ToolInfo Info() const override;
  void Press(const ToolInput& input) override;
  void Drag(const ToolInput& input) override;
  void Release(const ToolInput& input) override;
  void Cancel() override;
  void DrawOverlay(QPainter& painter, const ViewTransform& view) const override;
  bool IsBusy() const override { return mode_ != Mode::kIdle; }

 private:
  enum class Mode : std::uint8_t { kIdle, kMove, kTurn, kScale };

  // The art of the active layer, in layer space.
  std::optional<QRectF> ContentBox() const;
  // True when the layer has never been moved, turned or scaled.
  bool IsUntouched(const Layer& layer) const;
  // A canvas spot in the space the active layer's transform sits in.
  QPointF InOuterSpace(const QPointF& canvas) const;
  void Turn(const ToolInput& input);
  void Scale(const ToolInput& input);
  void Place(const LayerTransform& transform);

  Mode mode_ = Mode::kIdle;
  Id layer_ = kNoId;
  QPointF press_canvas_;
  LayerTransform start_{};
  QPointF origin_;
  // Set when this drag gives the layer its first origin.
  std::optional<QPointF> new_origin_;
  // Scaling: the corner that stays put and the one being dragged, in
  // layer space.
  QPointF anchor_;
  QPointF handle_;
  int drag_tag_ = 0;
  bool has_moved_ = false;
};

}  // namespace snapper
