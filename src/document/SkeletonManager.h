#pragma once

#include <QObject>
#include <QPointF>
#include <QString>
#include <QTransform>

#include <optional>
#include <vector>

#include "document/Ids.h"

namespace snapper {

// Where a bone sits, turns and stretches. On frame 0 it is the bone's own;
// other frames take it from the bone's channels.
struct BoneTransform {
  // Where its base sits: in its parent bone's space, with the parent's base
  // at 0 and the parent running along x, or in the bone layer's space for
  // a bone with no parent.
  QPointF position{};
  // Radians, from its parent's direction.
  double angle = 0.0;
  // Stretches it along its length, with the art that follows it.
  double scale = 1.0;

  bool operator==(const BoneTransform&) const = default;
  // Bone space to its parent's space: the base at 0, the bone along x.
  QTransform Matrix() const;
};

struct Bone {
  Id id = kNoId;
  Id layer = kNoId;  // the bone layer it belongs to
  Id parent = kNoId;
  QString name{};
  BoneTransform transform{};
  double length = 100.0;
  // IK stops at a pinned bone, which stays where it is.
  bool pinned = false;

  bool operator==(const Bone&) const = default;
};

struct RemovedBone {
  Bone bone{};
  int index = 0;
};

// Owns the bones of every bone layer, in one flat list where a parent
// always comes before its children, so posing is one forward loop.
class SkeletonManager : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMaxBonesPerSkeleton = 512;
  static constexpr int kMaxBones = 16384;
  static constexpr double kMinLength = 1.0;

  explicit SkeletonManager(QObject* parent = nullptr);

  // Inserts at index (clamped). Fails on a missing or repeated id, a full
  // skeleton, or a parent that is missing, on another layer, or not
  // before the index.
  bool Insert(const Bone& bone, int index);
  // Fails on a bone that has children.
  std::optional<RemovedBone> Take(Id id);
  // Replaces a bone's name, transform, length, pin and parent. Its id and
  // layer stay. Fails on a missing bone or a parent that is missing, on
  // another layer, not before it, or a bone inside it.
  bool Set(const Bone& bone);
  void Clear();

  const Bone* Find(Id id) const;
  // -1 when no bone has this id.
  int IndexOf(Id id) const;
  int Count() const { return static_cast<int>(bones_.size()); }
  const std::vector<Bone>& All() const { return bones_; }
  std::vector<Id> BonesOn(Id layer) const;
  std::vector<Id> ChildrenOf(Id bone) const;
  // True when `bone` is `ancestor` or sits below it.
  bool IsUnder(Id bone, Id ancestor) const;

 Q_SIGNALS:
  // Bones on this layer were added, removed or changed. kNoId for all.
  void BonesChanged(Id layer);

 private:
  bool IsGoodParent(const Bone& bone, int index) const;

  std::vector<Bone> bones_;
};

}  // namespace snapper
