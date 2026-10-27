#pragma once

#include <QUndoCommand>

#include <memory>
#include <vector>

#include "document/BindingManager.h"
#include "document/SkeletonManager.h"

namespace snapper {

class Document;

class AddBoneCommand : public QUndoCommand {
 public:
  AddBoneCommand(Document& document, const Bone& bone);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  Bone bone_;
};

// Puts bones' setup (name, transform, length, pin, parent) back and forth.
// Steps of one drag, sharing a tag other than 0, merge into one undo step.
class SetBonesCommand : public QUndoCommand {
 public:
  static constexpr int kMergeId = 7;

  SetBonesCommand(Document& document, const QString& text,
                  std::vector<Bone> before, std::vector<Bone> after,
                  int drag_tag = 0);

  void redo() override;
  void undo() override;
  int id() const override { return kMergeId; }
  bool mergeWith(const QUndoCommand* other) override;

 private:
  Document& document_;
  std::vector<Bone> before_;
  std::vector<Bone> after_;
  int drag_tag_;
};

// Deletes bones, Moho style: their children stay where they are, moved
// onto the nearest bone left above them, and layers bound to them come
// loose.
class RemoveBonesCommand : public QUndoCommand {
 public:
  RemoveBonesCommand(Document& document, const std::vector<Id>& bones);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  // In list order.
  std::vector<RemovedBone> removed_;
  std::vector<Bone> children_before_;
  std::vector<Bone> children_after_;
  std::vector<LayerBinding> bindings_;
};

// Binds a layer to a bone, or unbinds it with kNoId.
class BindLayerCommand : public QUndoCommand {
 public:
  BindLayerCommand(Document& document, Id layer, Id bone);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  Id layer_;
  Id old_bone_;
  Id new_bone_;
};

// Null when none of the bones exist.
std::unique_ptr<QUndoCommand> MakeRemoveBones(Document& document,
                                              const std::vector<Id>& bones);

}  // namespace snapper
