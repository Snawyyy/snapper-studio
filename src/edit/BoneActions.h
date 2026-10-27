#pragma once

#include <QString>

#include <functional>

#include "document/Ids.h"

namespace snapper {

class CommandRegistry;
class Document;
class SelectionManager;
class UndoManager;

// Whole-bone edits the bone options and the command list share: rename
// and pin. Each goes through undo.
class BoneActions {
 public:
  // `hint` shows why an action could not run.
  BoneActions(Document& document, SelectionManager& selection,
              UndoManager& undo, std::function<void(const QString&)> hint);

  // The one selected bone, or kNoId for none or several.
  Id SingleSelected() const;
  void RenameBone(Id bone, const QString& name);
  void SetPinned(Id bone, bool pinned);
  // Pins the selected bones, or unpins them when they all are pinned.
  void TogglePinSelected();

  // Pin or Unpin Bone.
  void RegisterCommands(CommandRegistry& commands);

 private:
  Document& document_;
  SelectionManager& selection_;
  UndoManager& undo_;
  std::function<void(const QString&)> hint_;
};

}  // namespace snapper
