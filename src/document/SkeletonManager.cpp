#include "document/SkeletonManager.h"

#include <algorithm>

namespace snapper {

QTransform BoneTransform::Matrix() const {
  QTransform matrix;
  matrix.translate(position.x(), position.y());
  matrix.rotateRadians(angle);
  return matrix;
}

SkeletonManager::SkeletonManager(QObject* parent) : QObject(parent) {}

bool SkeletonManager::Insert(const Bone& bone, int index) {
  const bool is_bad_id =
      bone.id == kNoId || bone.layer == kNoId || IndexOf(bone.id) >= 0;
  const bool is_full =
      Count() >= kMaxBones ||
      static_cast<int>(BonesOn(bone.layer).size()) >= kMaxBonesPerSkeleton;
  const int at = std::clamp(index, 0, Count());
  if (is_bad_id || is_full || !IsGoodParent(bone, at)) {
    return false;
  }
  // A child already in the list must stay after its parent.
  const bool has_child_before =
      std::any_of(bones_.begin(), bones_.begin() + at,
                  [&](const Bone& other) { return other.parent == bone.id; });
  if (has_child_before) {
    return false;
  }
  Bone added = bone;
  added.length = std::max(added.length, kMinLength);
  bones_.insert(bones_.begin() + at, added);
  Q_EMIT BonesChanged(bone.layer);
  return true;
}

std::optional<RemovedBone> SkeletonManager::Take(Id id) {
  const int index = IndexOf(id);
  if (index < 0 || !ChildrenOf(id).empty()) {
    return std::nullopt;
  }
  RemovedBone removed{.bone = bones_[static_cast<size_t>(index)],
                      .index = index};
  bones_.erase(bones_.begin() + index);
  Q_EMIT BonesChanged(removed.bone.layer);
  return removed;
}

bool SkeletonManager::Set(const Bone& bone) {
  const int index = IndexOf(bone.id);
  if (index < 0) {
    return false;
  }
  Bone& found = bones_[static_cast<size_t>(index)];
  Bone changed = bone;
  changed.layer = found.layer;
  const bool is_loop =
      changed.parent != kNoId && IsUnder(changed.parent, bone.id);
  if (is_loop || !IsGoodParent(changed, index)) {
    return false;
  }
  changed.length = std::max(changed.length, kMinLength);
  if (found == changed) {
    return true;
  }
  found = std::move(changed);
  Q_EMIT BonesChanged(found.layer);
  return true;
}

void SkeletonManager::Clear() {
  bones_.clear();
  Q_EMIT BonesChanged(kNoId);
}

const Bone* SkeletonManager::Find(Id id) const {
  const int index = IndexOf(id);
  return index < 0 ? nullptr : &bones_[static_cast<size_t>(index)];
}

int SkeletonManager::IndexOf(Id id) const {
  if (id == kNoId) {
    return -1;
  }
  for (size_t i = 0; i < bones_.size(); ++i) {
    if (bones_[i].id == id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

std::vector<Id> SkeletonManager::BonesOn(Id layer) const {
  std::vector<Id> ids;
  for (const Bone& bone : bones_) {
    if (bone.layer == layer) {
      ids.push_back(bone.id);
    }
  }
  return ids;
}

std::vector<Id> SkeletonManager::ChildrenOf(Id bone) const {
  std::vector<Id> ids;
  for (const Bone& other : bones_) {
    if (other.parent == bone) {
      ids.push_back(other.id);
    }
  }
  return ids;
}

bool SkeletonManager::IsUnder(Id bone, Id ancestor) const {
  const Bone* found = Find(bone);
  for (int steps = 0; found != nullptr && steps <= Count(); ++steps) {
    if (found->id == ancestor) {
      return true;
    }
    found = Find(found->parent);
  }
  return false;
}

bool SkeletonManager::IsGoodParent(const Bone& bone, int index) const {
  if (bone.parent == kNoId) {
    return true;
  }
  const int parent = IndexOf(bone.parent);
  const bool is_good = parent >= 0 && parent < index &&
                       bones_[static_cast<size_t>(parent)].layer == bone.layer;
  return is_good;
}

}  // namespace snapper
