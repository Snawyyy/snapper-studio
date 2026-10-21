#include "edit/LayerCommands.h"

#include "document/Document.h"
#include "edit/ShapeCommands.h"

namespace snapper {

AddLayerCommand::AddLayerCommand(Document& document, const Layer& layer,
                                 int index)
    : QUndoCommand(QStringLiteral("Add Layer")),
      document_(document),
      layer_(layer),
      index_(index) {}

void AddLayerCommand::redo() { document_.Layers().Insert(layer_, index_); }
void AddLayerCommand::undo() { document_.Layers().Take(layer_.id); }

RemoveLayerCommand::RemoveLayerCommand(Document& document, Id layer)
    : QUndoCommand(QStringLiteral("Delete Layer")), document_(document) {
  const LayerManager& layers = document.Layers();
  const Layer* found = layers.Find(layer);
  if (found != nullptr) {
    layer_ = {.layer = *found, .index = layers.IndexOf(layer)};
  }
  const ShapeManager& shapes = document.Shapes();
  for (const Id shape : shapes.ShapesOn(layer)) {
    shapes_.push_back(
        {.shape = *shapes.Find(shape), .index = shapes.IndexOf(shape)});
  }
}

void RemoveLayerCommand::redo() {
  SwapShapes(document_.Shapes(), shapes_, {});
  document_.Layers().Take(layer_.layer.id);
}

void RemoveLayerCommand::undo() {
  document_.Layers().Insert(layer_.layer, layer_.index);
  SwapShapes(document_.Shapes(), {}, shapes_);
}

RenameLayerCommand::RenameLayerCommand(Document& document, Id layer,
                                       const QString& name)
    : QUndoCommand(QStringLiteral("Rename Layer")),
      document_(document),
      layer_(layer),
      new_name_(name) {
  const Layer* found = document.Layers().Find(layer);
  if (found != nullptr) {
    old_name_ = found->name;
  }
}

void RenameLayerCommand::redo() {
  document_.Layers().SetName(layer_, new_name_);
}

void RenameLayerCommand::undo() {
  document_.Layers().SetName(layer_, old_name_);
}

SetLayerVisibleCommand::SetLayerVisibleCommand(Document& document, Id layer,
                                               bool visible)
    : QUndoCommand(visible ? QStringLiteral("Show Layer")
                           : QStringLiteral("Hide Layer")),
      document_(document),
      layer_(layer),
      visible_(visible) {}

void SetLayerVisibleCommand::redo() {
  document_.Layers().SetVisible(layer_, visible_);
}

void SetLayerVisibleCommand::undo() {
  document_.Layers().SetVisible(layer_, !visible_);
}

SetLayerLockedCommand::SetLayerLockedCommand(Document& document, Id layer,
                                             bool locked)
    : QUndoCommand(locked ? QStringLiteral("Lock Layer")
                          : QStringLiteral("Unlock Layer")),
      document_(document),
      layer_(layer),
      locked_(locked) {}

void SetLayerLockedCommand::redo() {
  document_.Layers().SetLocked(layer_, locked_);
}

void SetLayerLockedCommand::undo() {
  document_.Layers().SetLocked(layer_, !locked_);
}

MoveLayerCommand::MoveLayerCommand(Document& document, Id layer, int to_index)
    : QUndoCommand(QStringLiteral("Move Layer")),
      document_(document),
      layer_(layer),
      from_index_(document.Layers().IndexOf(layer)),
      to_index_(to_index) {}

void MoveLayerCommand::redo() { document_.Layers().Move(layer_, to_index_); }
void MoveLayerCommand::undo() { document_.Layers().Move(layer_, from_index_); }

}  // namespace snapper
