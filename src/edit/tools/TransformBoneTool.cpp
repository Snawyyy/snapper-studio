#include "edit/tools/TransformBoneTool.h"

#include <QLineF>

#include <algorithm>
#include <cmath>
#include <numbers>

#include "document/Document.h"
#include "edit/AnimationEditor.h"
#include "edit/BoneCommands.h"
#include "edit/HitTest.h"
#include "edit/SelectionCommands.h"
#include "edit/SelectionManager.h"
#include "edit/ShapeCommands.h"
#include "edit/UndoManager.h"
#include "edit/tools/LayerCheck.h"
#include "eval/FrameEvaluator.h"

namespace snapper {

namespace {

constexpr double kSnapAngle = std::numbers::pi / 12;
constexpr double kMinScale = 0.05;

double Wrapped(double angle) {
  return std::remainder(angle, 2 * std::numbers::pi);
}

double HeadingOf(const QPointF& span) { return std::atan2(span.y(), span.x()); }

}  // namespace

TransformBoneTool::TransformBoneTool(ToolContext context)
    : Tool(std::move(context)) {}

ToolInfo TransformBoneTool::Info() const {
  return {.id = QStringLiteral("transform-bone"),
          .title = QStringLiteral("Transform Bone"),
          .key = QKeySequence(Qt::Key_T),
          .hint = QStringLiteral(
              "Transform Bone: drag a bone to move it, its tip to turn it, "
              "Alt+drag to stretch it. Shift snaps."),
          .is_for_bone_layers = true};
}

void TransformBoneTool::Press(const ToolInput& input) {
  const Id bone_layer =
      BoneLayerToEdit(Context(), QStringLiteral("transform bones on"));
  if (bone_layer == kNoId) {
    return;
  }
  const Pose& pose = Context().frames.Current();
  const BoneHit hit =
      HitTest::BoneAt(pose, bone_layer, *input.view, input.screen);
  SelectionManager& selection = Context().selection;
  if (hit.bone == kNoId) {
    Context().undo.Push(MakeSelectBones(selection, {}));
    return;
  }
  if (!selection.IsBoneSelected(hit.bone)) {
    Context().undo.Push(MakeSelectBones(selection, {hit.bone}));
  }
  const PosedBone* posed = pose.FindBone(hit.bone);
  const Bone* bone = Context().document.Skeletons().Find(hit.bone);
  bone_layer_ = bone_layer;
  bone_ = hit.bone;
  start_ = posed->transform;
  start_length_ = bone->length;
  start_heading_ = posed->Heading();
  start_base_ = posed->Base();
  press_ = InBoneLayer(Context(), bone_layer, input.canvas);
  drag_tag_ = MovePointsCommand::NewDragTag();
  has_moved_ = false;
  if (input.modifiers.testFlag(Qt::AltModifier)) {
    mode_ = Mode::kStretch;
  } else {
    mode_ = hit.part == BonePart::kTip ? Mode::kTurn : Mode::kMove;
  }
}

void TransformBoneTool::Drag(const ToolInput& input) {
  if (mode_ == Mode::kIdle) {
    return;
  }
  const QPointF spot = InBoneLayer(Context(), bone_layer_, input.canvas);
  const bool is_shift = input.modifiers.testFlag(Qt::ShiftModifier);
  switch (mode_) {
    case Mode::kMove:
      Move(spot, is_shift);
      break;
    case Mode::kTurn:
      Turn(spot, is_shift);
      break;
    case Mode::kStretch:
      Stretch(spot);
      break;
    case Mode::kIdle:
      break;
  }
}

void TransformBoneTool::Move(const QPointF& spot, bool is_on_one_axis) {
  QPointF delta = spot - press_;
  if (is_on_one_axis) {
    const bool is_sideways = std::abs(delta.x()) >= std::abs(delta.y());
    delta = is_sideways ? QPointF(delta.x(), 0) : QPointF(0, delta.y());
  }
  const Bone* bone = Context().document.Skeletons().Find(bone_);
  const PosedBone* parent =
      bone == nullptr ? nullptr
                      : Context().frames.Current().FindBone(bone->parent);
  BoneTransform moved = start_;
  const QPointF base = start_base_ + delta;
  moved.position = parent == nullptr ? base : parent->ChildPosition(base);
  Place(moved, start_length_);
}

void TransformBoneTool::Turn(const QPointF& spot, bool is_snapped) {
  double turn =
      Wrapped(HeadingOf(spot - start_base_) - HeadingOf(press_ - start_base_));
  if (is_snapped) {
    const double heading =
        std::round((start_heading_ + turn) / kSnapAngle) * kSnapAngle;
    turn = heading - start_heading_;
  }
  BoneTransform turned = start_;
  turned.angle = start_.angle + turn;
  // On the setup frame the tip follows the pointer, so the length changes
  // by how much farther from the base it is than at the press.
  double length = start_length_;
  const double reach = QLineF(start_base_, press_).length();
  const bool sets_length = Context().animation.IsSetupFrame() && reach > 1e-6;
  if (sets_length) {
    length =
        std::max(SkeletonManager::kMinLength,
                 start_length_ * QLineF(start_base_, spot).length() / reach);
  }
  Place(turned, length);
}

void TransformBoneTool::Stretch(const QPointF& spot) {
  const double reach = QLineF(start_base_, press_).length();
  if (reach <= 1e-6) {
    return;
  }
  BoneTransform stretched = start_;
  stretched.scale = std::max(
      kMinScale, start_.scale * QLineF(start_base_, spot).length() / reach);
  Place(stretched, start_length_);
}

void TransformBoneTool::Place(const BoneTransform& transform, double length) {
  has_moved_ = true;
  const Bone* bone = Context().document.Skeletons().Find(bone_);
  if (bone == nullptr) {
    return;
  }
  const bool is_new_length =
      Context().animation.IsSetupFrame() && length != bone->length;
  if (!is_new_length) {
    Context().undo.Push(Context().animation.PlaceBones(
        {{.bone = bone_, .transform = transform}}, drag_tag_));
    return;
  }
  Bone changed = *bone;
  changed.transform = transform;
  changed.length = length;
  Context().undo.Push(std::make_unique<SetBonesCommand>(
      Context().document, QStringLiteral("Transform Bones"),
      std::vector<Bone>{*bone}, std::vector<Bone>{changed}, drag_tag_));
}

void TransformBoneTool::Release(const ToolInput& /*input*/) {
  mode_ = Mode::kIdle;
}

// Escape mid-drag puts the bone back.
void TransformBoneTool::Cancel() {
  const bool is_mid_drag = mode_ != Mode::kIdle && has_moved_;
  if (is_mid_drag) {
    Context().undo.Undo();
  }
  mode_ = Mode::kIdle;
}

}  // namespace snapper
