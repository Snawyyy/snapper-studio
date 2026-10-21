#include "edit/SelectionManager.h"

#include "document/Document.h"

namespace snapper {

SelectionManager::SelectionManager(Document& document, QObject* parent)
    : QObject(parent), document_(document) {
  connect(&document_.Layers(), &LayerManager::ListChanged, this,
          &SelectionManager::KeepActiveLayerValid);
  connect(&document_, &Document::Reset, this, [this] {
    active_layer_ = kNoId;
    KeepActiveLayerValid();
  });
  KeepActiveLayerValid();
}

bool SelectionManager::SetActiveLayer(Id layer) {
  if (document_.Layers().Find(layer) == nullptr) {
    return false;
  }
  if (layer != active_layer_) {
    active_layer_ = layer;
    Q_EMIT ActiveLayerChanged(layer);
  }
  return true;
}

void SelectionManager::KeepActiveLayerValid() {
  const LayerManager& layers = document_.Layers();
  const bool is_valid = layers.Find(active_layer_) != nullptr;
  if (is_valid) {
    return;
  }
  active_layer_ = layers.Count() == 0 ? kNoId : layers.All().back().id;
  Q_EMIT ActiveLayerChanged(active_layer_);
}

}  // namespace snapper
