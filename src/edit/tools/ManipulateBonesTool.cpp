#include "edit/tools/ManipulateBonesTool.h"

#include <algorithm>

#include "document/Document.h"
#include "edit/AnimationEditor.h"
#include "edit/HitTest.h"
#include "edit/SelectionCommands.h"
#include "edit/SelectionManager.h"
#include "edit/ShapeCommands.h"
#include "edit/UndoManager.h"
#include "edit/tools/LayerCheck.h"
#include "eval/FrameEvaluator.h"
#include "eval/IkSolver.h"

namespace snapper {

ManipulateBonesTool::ManipulateBonesTool(ToolContext context)
    : Tool(std::move(context)) {}

ToolInfo ManipulateBonesTool::Info() const {
  return {.id = QStringLiteral("manipulate-bones"),
          .title = QStringLiteral("Manipulate Bones"),
          .key = QKeySequence(Qt::Key_Z),
          .hint = QStringLiteral(
              "Manipulate Bones: pull a bone and the chain above it follows, "
              "up to a pinned bone. On the setup frame it only tests the "
              "rig.")};
}

void ManipulateBonesTool::Press(const ToolInput& input) {
  const Id bone_layer =
      BoneLayerToEdit(Context(), QStringLiteral("pose bones on"));
  if (bone_layer == kNoId) {
    return;
  }
  const Pose& pose = Context().frames.Current();
  const BoneHit hit =
      HitTest::BoneAt(pose, bone_layer, *input.view, input.screen);
  if (hit.bone == kNoId) {
    Context().undo.Push(MakeSelectBones(Context().selection, {}));
    return;
  }
  Context().undo.Push(MakeSelectBones(Context().selection, {hit.bone}));
  const SkeletonManager& skeletons = Context().document.Skeletons();
  const Bone* grabbed = skeletons.Find(hit.bone);
  const PosedBone* posed = pose.FindBone(hit.bone);
  bone_layer_ = bone_layer;
  bone_ = hit.bone;
  grab_ = hit.along * grabbed->length * posed->transform.scale;
  // Up the chain, stopping below a pinned bone. A pinned bone grabbed
  // itself still turns on its own.
  chain_ = {hit.bone};
  const Bone* above = skeletons.Find(grabbed->parent);
  const int limit = skeletons.Count();
  for (int steps = 0; above != nullptr && !above->pinned && steps < limit;
       ++steps) {
    chain_.push_back(above->id);
    above = skeletons.Find(above->parent);
  }
  std::ranges::reverse(chain_);
  drag_tag_ = MovePointsCommand::NewDragTag();
  has_moved_ = false;
}

void ManipulateBonesTool::Drag(const ToolInput& input) {
  if (bone_ == kNoId) {
    return;
  }
  // Each step bends on from where the last one left the bones, so the
  // pose never jumps between solutions.
  const Pose& pose = Context().frames.Current();
  std::vector<QPointF> bases;
  for (const Id id : chain_) {
    bases.push_back(pose.FindBone(id)->Base());
  }
  const QPointF grabbed = pose.FindBone(bone_)->matrix.map(QPointF(grab_, 0));
  const std::vector<double> turns =
      IkSolver::Solve(std::move(bases), grabbed,
                      InBoneLayer(Context(), bone_layer_, input.canvas));
  std::vector<BonePlacement> placements;
  for (size_t i = 0; i < chain_.size(); ++i) {
    // Turning round a joint turns that bone and carries everything below
    // it, so only that bone's own angle changes.
    BoneTransform transform = pose.FindBone(chain_[i])->transform;
    transform.angle += turns[i];
    placements.push_back({.bone = chain_[i], .transform = transform});
  }
  has_moved_ = true;
  if (Context().animation.IsSetupFrame()) {
    QHash<Id, BoneTransform> trial = Context().frames.Trial();
    for (const BonePlacement& placement : placements) {
      trial.insert(placement.bone, placement.transform);
    }
    Context().frames.SetTrial(std::move(trial));
    return;
  }
  Context().undo.Push(Context().animation.PlaceBones(placements, drag_tag_));
}

void ManipulateBonesTool::Release(const ToolInput& /*input*/) { Finish(false); }

void ManipulateBonesTool::Cancel() { Finish(true); }

void ManipulateBonesTool::Finish(bool is_cancel) {
  if (bone_ == kNoId) {
    return;
  }
  const bool was_test = Context().animation.IsSetupFrame();
  if (was_test) {
    Context().frames.SetTrial({});
    if (has_moved_ && !is_cancel) {
      Context().hint(QStringLiteral(
          "The setup frame only tests the rig. Go to a later frame to pose "
          "it"));
    }
  } else if (is_cancel && has_moved_) {
    Context().undo.Undo();
  }
  bone_ = kNoId;
  chain_.clear();
}

}  // namespace snapper
