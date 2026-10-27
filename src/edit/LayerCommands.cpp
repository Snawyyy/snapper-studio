#include "edit/LayerCommands.h"

#include <algorithm>

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
    : QUndoCommand(QStringLiteral("Delete Layer")),
      document_(document),
      layer_(layer) {
  const LayerManager& layers = document.Layers();
  const int from = layers.IndexOf(layer);
  const int end = layers.EndOf(layer);
  const ShapeManager& shapes = document.Shapes();
  for (int i = std::max(from, 0); i < end; ++i) {
    const Layer& inside = layers.All()[static_cast<size_t>(i)];
    layers_.push_back({.layer = inside, .index = i});
    for (const Id shape : shapes.ShapesOn(inside.id)) {
      shapes_.push_back(
          {.shape = *shapes.Find(shape), .index = shapes.IndexOf(shape)});
    }
  }
  // Shapes go back in list order, whatever layer they are on.
  std::ranges::sort(shapes_, {}, &RemovedShape::index);
}

void RemoveLayerCommand::redo() {
  SwapShapes(document_.Shapes(), shapes_, {});
  document_.Layers().TakeTree(layer_);
}

void RemoveLayerCommand::undo() {
  for (const RemovedLayer& removed : layers_) {
    document_.Layers().Insert(removed.layer, removed.index);
  }
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

MoveLayerCommand::MoveLayerCommand(Document& document, Id layer, Id to_parent,
                                   int to_index)
    : QUndoCommand(QStringLiteral("Move Layer")),
      document_(document),
      layer_(layer),
      from_index_(document.Layers().IndexOf(layer)),
      to_parent_(to_parent),
      to_index_(to_index) {
  const Layer* found = document.Layers().Find(layer);
  if (found != nullptr) {
    from_parent_ = found->parent;
  }
}

void MoveLayerCommand::redo() {
  document_.Layers().Place(layer_, to_parent_, to_index_);
}

void MoveLayerCommand::undo() {
  document_.Layers().Place(layer_, from_parent_, from_index_);
}

}  // namespace snapper
