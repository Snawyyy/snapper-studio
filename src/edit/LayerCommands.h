#pragma once

#include <QUndoCommand>

#include <vector>

#include "document/LayerManager.h"
#include "document/ShapeManager.h"

namespace snapper {

class Document;

// Adds a layer at an index in the layer list.
class AddLayerCommand : public QUndoCommand {
 public:
  AddLayerCommand(Document& document, const Layer& layer, int index);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  Layer layer_;
  int index_;
};

// Removes a layer along with everything drawn on it.
class RemoveLayerCommand : public QUndoCommand {
 public:
  RemoveLayerCommand(Document& document, Id layer);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  RemovedLayer layer_;
  std::vector<RemovedShape> shapes_;
};

class RenameLayerCommand : public QUndoCommand {
 public:
  RenameLayerCommand(Document& document, Id layer, const QString& name);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  Id layer_;
  QString old_name_;
  QString new_name_;
};

class SetLayerVisibleCommand : public QUndoCommand {
 public:
  SetLayerVisibleCommand(Document& document, Id layer, bool visible);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  Id layer_;
  bool visible_;
};

}  // namespace snapper
