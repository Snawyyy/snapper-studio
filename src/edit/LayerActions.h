#pragma once

#include <QString>

#include <functional>

#include "document/Ids.h"

namespace snapper {

class CommandRegistry;
class Document;
class SelectionManager;
class UndoManager;

// Everything you can do to layers as a whole: add, delete, rename, show,
// lock and restack. Each goes through undo. The Layers panel and the
// command list both call in here.
class LayerActions {
 public:
  // `hint` shows why an action could not run.
  LayerActions(Document& document, SelectionManager& selection,
               UndoManager& undo, std::function<void(const QString&)> hint);

  // Adds a vector layer above the active one and makes it active.
  void AddLayer();
  // Fails, with a reason, on the last layer: a project keeps one.
  bool DeleteLayer(Id layer, QString* why_not);
  void RenameLayer(Id layer, const QString& name);
  void SetVisible(Id layer, bool visible);
  void SetLocked(Id layer, bool locked);
  // +1 moves up one, -1 down one.
  void MoveLayer(Id layer, int steps);

  // New Layer, Delete Layer, Lock Layer, Move Layer Up and Down. Rename
  // needs the panel and is added there.
  void RegisterCommands(CommandRegistry& commands);

 private:
  QString UnusedName() const;

  Document& document_;
  SelectionManager& selection_;
  UndoManager& undo_;
  std::function<void(const QString&)> hint_;
};

}  // namespace snapper
