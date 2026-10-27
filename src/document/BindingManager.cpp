#include "document/BindingManager.h"

#include <algorithm>

namespace snapper {

BindingManager::BindingManager(QObject* parent) : QObject(parent) {}

bool BindingManager::BindLayer(Id layer, Id bone) {
  if (layer == kNoId) {
    return false;
  }
  if (bone == kNoId) {
    const bool was_bound = layer_bones_.remove(layer);
    if (was_bound) {
      Q_EMIT LayerBindingChanged(layer);
    }
    return true;
  }
  const bool is_new = !layer_bones_.contains(layer);
  const bool is_full = is_new && layer_bones_.size() >= kMaxLayerBindings;
  if (is_full) {
    return false;
  }
  if (layer_bones_.value(layer, kNoId) == bone) {
    return true;
  }
  layer_bones_.insert(layer, bone);
  Q_EMIT LayerBindingChanged(layer);
  return true;
}

void BindingManager::Clear() {
  layer_bones_.clear();
  Q_EMIT LayerBindingChanged(kNoId);
}

std::vector<Id> BindingManager::LayersOnBone(Id bone) const {
  std::vector<Id> layers;
  for (auto it = layer_bones_.cbegin(); it != layer_bones_.cend(); ++it) {
    if (it.value() == bone) {
      layers.push_back(it.key());
    }
  }
  std::ranges::sort(layers);
  return layers;
}

std::vector<LayerBinding> BindingManager::LayerBindings() const {
  std::vector<LayerBinding> bindings;
  for (auto it = layer_bones_.cbegin(); it != layer_bones_.cend(); ++it) {
    bindings.push_back({.layer = it.key(), .bone = it.value()});
  }
  std::ranges::sort(bindings, {}, &LayerBinding::layer);
  return bindings;
}

}  // namespace snapper
