#include "edit/BoneCommands.h"

#include <QSet>

#include <algorithm>
#include <cmath>

#include "document/Document.h"
#include "eval/FrameEvaluator.h"

namespace snapper {

AddBoneCommand::AddBoneCommand(Document& document, const Bone& bone)
    : QUndoCommand(QStringLiteral("Add Bone")),
      document_(document),
      bone_(bone) {}

void AddBoneCommand::redo() {
  document_.Skeletons().Insert(bone_, document_.Skeletons().Count());
}

void AddBoneCommand::undo() { document_.Skeletons().Take(bone_.id); }

SetBonesCommand::SetBonesCommand(Document& document, const QString& text,
                                 std::vector<Bone> before,
                                 std::vector<Bone> after, int drag_tag)
    : QUndoCommand(text),
      document_(document),
      before_(std::move(before)),
      after_(std::move(after)),
      drag_tag_(drag_tag) {}

void SetBonesCommand::redo() {
  for (const Bone& bone : after_) {
    document_.Skeletons().Set(bone);
  }
}

void SetBonesCommand::undo() {
  for (const Bone& bone : before_) {
    document_.Skeletons().Set(bone);
  }
}

bool SetBonesCommand::mergeWith(const QUndoCommand* other) {
  if (other->id() != kMergeId) {
    return false;
  }
  // Matching ids make this cast safe. A later step may touch bones this
  // one didn't, so those join in.
  const auto* next = static_cast<const SetBonesCommand*>(other);
  const bool is_same_drag = drag_tag_ != 0 && next->drag_tag_ == drag_tag_;
  if (!is_same_drag) {
    return false;
  }
  for (size_t i = 0; i < next->after_.size(); ++i) {
    const Bone& bone = next->after_[i];
    const auto found = std::ranges::find(after_, bone.id, &Bone::id);
    if (found != after_.end()) {
      *found = bone;
      continue;
    }
    after_.push_back(bone);
    before_.push_back(next->before_[i]);
  }
  return true;
}

RemoveBonesCommand::RemoveBonesCommand(Document& document,
                                       const std::vector<Id>& bones)
    : QUndoCommand(QStringLiteral("Delete Bones")), document_(document) {
  const SkeletonManager& skeletons = document.Skeletons();
  const QSet<Id> going(bones.begin(), bones.end());
  for (const Bone& bone : skeletons.All()) {
    if (going.contains(bone.id)) {
      removed_.push_back({.bone = bone, .index = skeletons.IndexOf(bone.id)});
    }
  }
  // Each kept child of a deleted bone moves onto the nearest bone kept
  // above it, keeping its setup place.
  const Pose setup = FrameEvaluator::Evaluate(document, 0);
  for (const Bone& bone : skeletons.All()) {
    const bool is_orphaned =
        !going.contains(bone.id) && going.contains(bone.parent);
    if (!is_orphaned) {
      continue;
    }
    Id parent = bone.parent;
    while (going.contains(parent)) {
      parent = skeletons.Find(parent)->parent;
    }
    Bone moved = bone;
    moved.parent = parent;
    const PosedBone* posed = setup.FindBone(bone.id);
    const PosedBone* new_parent = setup.FindBone(parent);
    QTransform local = posed->setup_matrix;
    if (new_parent != nullptr) {
      local = local * new_parent->setup_matrix.inverted();
    }
    QPointF position = local.map(QPointF(0, 0));
    if (new_parent != nullptr) {
      position.setX(position.x() / new_parent->setup_scale);
    }
    moved.transform.position = position;
    moved.transform.angle = std::atan2(local.m12(), local.m11());
    children_before_.push_back(bone);
    children_after_.push_back(moved);
  }
  for (const RemovedBone& removed : removed_) {
    for (const Id layer : document.Bindings().LayersOnBone(removed.bone.id)) {
      bindings_.push_back({.layer = layer, .bone = removed.bone.id});
    }
  }
  setText(removed_.size() == 1 ? QStringLiteral("Delete Bone")
                               : QStringLiteral("Delete Bones"));
}

void RemoveBonesCommand::redo() {
  for (const LayerBinding& binding : bindings_) {
    document_.Bindings().BindLayer(binding.layer, kNoId);
  }
  for (const Bone& child : children_after_) {
    document_.Skeletons().Set(child);
  }
  // Children first, so no bone taken still has one.
  for (auto it = removed_.rbegin(); it != removed_.rend(); ++it) {
    document_.Skeletons().Take(it->bone.id);
  }
}

void RemoveBonesCommand::undo() {
  for (const RemovedBone& removed : removed_) {
    document_.Skeletons().Insert(removed.bone, removed.index);
  }
  for (const Bone& child : children_before_) {
    document_.Skeletons().Set(child);
  }
  for (const LayerBinding& binding : bindings_) {
    document_.Bindings().BindLayer(binding.layer, binding.bone);
  }
}

BindLayerCommand::BindLayerCommand(Document& document, Id layer, Id bone)
    : QUndoCommand(bone == kNoId ? QStringLiteral("Unbind Layer")
                                 : QStringLiteral("Bind Layer")),
      document_(document),
      layer_(layer),
      old_bone_(document.Bindings().BoneOfLayer(layer)),
      new_bone_(bone) {}

void BindLayerCommand::redo() {
  document_.Bindings().BindLayer(layer_, new_bone_);
}

void BindLayerCommand::undo() {
  document_.Bindings().BindLayer(layer_, old_bone_);
}

std::unique_ptr<QUndoCommand> MakeRemoveBones(Document& document,
                                              const std::vector<Id>& bones) {
  const bool has_any = std::ranges::any_of(bones, [&](Id bone) {
    return document.Skeletons().Find(bone) != nullptr;
  });
  if (!has_any) {
    return nullptr;
  }
  return std::make_unique<RemoveBonesCommand>(document, bones);
}

}  // namespace snapper
