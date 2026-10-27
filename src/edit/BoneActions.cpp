#include "edit/BoneActions.h"

#include <algorithm>
#include <vector>

#include "app/CommandRegistry.h"
#include "document/Document.h"
#include "edit/BoneCommands.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"

namespace snapper {

BoneActions::BoneActions(Document& document, SelectionManager& selection,
                         UndoManager& undo,
                         std::function<void(const QString&)> hint)
    : document_(document),
      selection_(selection),
      undo_(undo),
      hint_(std::move(hint)) {}

Id BoneActions::SingleSelected() const {
  const QSet<Id>& bones = selection_.Bones();
  return bones.size() == 1 ? *bones.cbegin() : kNoId;
}

void BoneActions::RenameBone(Id bone, const QString& name) {
  const Bone* found = document_.Skeletons().Find(bone);
  const QString trimmed = name.trimmed();
  const bool is_change =
      found != nullptr && !trimmed.isEmpty() && found->name != trimmed;
  if (!is_change) {
    return;
  }
  Bone renamed = *found;
  renamed.name = trimmed;
  undo_.Push(std::make_unique<SetBonesCommand>(
      document_, QStringLiteral("Rename Bone"), std::vector<Bone>{*found},
      std::vector<Bone>{renamed}));
}

void BoneActions::SetPinned(Id bone, bool pinned) {
  const Bone* found = document_.Skeletons().Find(bone);
  const bool is_change = found != nullptr && found->pinned != pinned;
  if (!is_change) {
    return;
  }
  Bone changed = *found;
  changed.pinned = pinned;
  undo_.Push(std::make_unique<SetBonesCommand>(
      document_,
      pinned ? QStringLiteral("Pin Bone") : QStringLiteral("Unpin Bone"),
      std::vector<Bone>{*found}, std::vector<Bone>{changed}));
}

void BoneActions::TogglePinSelected() {
  std::vector<Bone> before;
  for (const Id id : selection_.Bones()) {
    const Bone* bone = document_.Skeletons().Find(id);
    if (bone != nullptr) {
      before.push_back(*bone);
    }
  }
  if (before.empty()) {
    hint_(QStringLiteral("Pin Bone: select a bone first"));
    return;
  }
  const bool are_all_pinned =
      std::ranges::all_of(before, [](const Bone& bone) { return bone.pinned; });
  std::vector<Bone> after = before;
  for (Bone& bone : after) {
    bone.pinned = !are_all_pinned;
  }
  undo_.Push(std::make_unique<SetBonesCommand>(
      document_,
      are_all_pinned ? QStringLiteral("Unpin Bones")
                     : QStringLiteral("Pin Bones"),
      std::move(before), std::move(after)));
}

void BoneActions::RegisterCommands(CommandRegistry& commands) {
  commands.Add({
      .id = QStringLiteral("bone.pin"),
      .title = QStringLiteral("Pin or Unpin Bone"),
      .shortcut = {},
      .run = [this] { TogglePinSelected(); },
  });
}

}  // namespace snapper
