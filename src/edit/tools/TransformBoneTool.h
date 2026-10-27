#pragma once

#include <QPointF>

#include <cstdint>

#include "document/SkeletonManager.h"
#include "edit/Tool.h"

namespace snapper {

// Moves, turns and stretches one bone, Moho style: drag its body to move
// it, its tip to turn it, Alt+drag to stretch it. On the setup frame this
// edits the rig, and turning by the tip sets its length too. On other
// frames it sets keys. Shift keeps a move on one axis and snaps turns to
// 15 degrees.
class TransformBoneTool : public Tool {
 public:
  explicit TransformBoneTool(ToolContext context);

  ToolInfo Info() const override;
  void Press(const ToolInput& input) override;
  void Drag(const ToolInput& input) override;
  void Release(const ToolInput& input) override;
  void Cancel() override;
  bool IsBusy() const override { return mode_ != Mode::kIdle; }

 private:
  enum class Mode : std::uint8_t { kIdle, kMove, kTurn, kStretch };

  void Move(const QPointF& spot, bool is_on_one_axis);
  void Turn(const QPointF& spot, bool is_snapped);
  void Stretch(const QPointF& spot);
  void Place(const BoneTransform& transform, double length);

  Mode mode_ = Mode::kIdle;
  Id bone_layer_ = kNoId;
  Id bone_ = kNoId;
  // As it stood at the press, in bone layer space.
  BoneTransform start_{};
  double start_length_ = 0.0;
  double start_heading_ = 0.0;
  QPointF start_base_;
  QPointF press_;
  int drag_tag_ = 0;
  bool has_moved_ = false;
};

}  // namespace snapper
