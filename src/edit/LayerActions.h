#pragma once

#include <QString>

#include <cstdint>
#include <functional>

#include "document/Ids.h"

namespace snapper {

// Where a layer dragged in the Layers panel lands, next to the row it was
// dropped on.
enum class DropSpot : std::uint8_t { kAbove, kBelow, kInto };

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

  // Adds a vector layer above the active one, beside it in the same
  // parent, and makes it active.
  void AddLayer();
  // Adds a bone layer where the active layer is and puts the active layer
  // inside it. The bone layer becomes active.
  void AddBoneLayer();
  // Fails, with a reason, when it would leave no layer: a project keeps
  // one. A bone layer goes with the layers inside it.
  bool DeleteLayer(Id layer, QString* why_not);
  void RenameLayer(Id layer, const QString& name);
  void SetVisible(Id layer, bool visible);
  void SetLocked(Id layer, bool locked);
  // +1 moves up one, -1 down one, past the layers beside it.
  void MoveLayer(Id layer, int steps);
  // Moves a layer into `parent` (kNoId for top level) at an index as for
  // LayerManager::Place. Fails, with a reason, where it can't go.
  bool PlaceLayer(Id layer, Id parent, int index, QString* why_not);
  // Moves a layer dragged onto another layer's row: above or below it,
  // beside it, or into it at the top when it is a bone layer. Hints why
  // when it can't.
  void DropLayer(Id layer, Id target, DropSpot spot);
  // Adds a picture from a file as an image layer at the bottom, centered
  // and shrunk to fit the canvas, ready to trace over. The active layer
  // stays active.
  bool ImportPicture(const QString& path, QString* why_not);

  // New Layer, Delete Layer, Lock Layer, Move Layer Up and Down. Rename
  // needs the panel and is added there.
  void RegisterCommands(CommandRegistry& commands);

 private:
  // "<base> <n>" no layer has yet.
  QString UnusedName(const QString& base) const;

  Document& document_;
  SelectionManager& selection_;
  UndoManager& undo_;
  std::function<void(const QString&)> hint_;
};

}  // namespace snapper
