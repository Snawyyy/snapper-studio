#include "document/LayerManager.h"

#include <algorithm>

namespace snapper {

LayerManager::LayerManager(QObject* parent) : QObject(parent) {}

bool LayerManager::Insert(const Layer& layer, int index) {
  const bool is_full = Count() >= kMaxLayers;
  const bool is_bad_id = layer.id == kNoId || IndexOf(layer.id) >= 0;
  if (is_full || is_bad_id) {
    return false;
  }
  const int at = std::clamp(index, 0, Count());
  layers_.insert(layers_.begin() + at, layer);
  Q_EMIT ListChanged();
  return true;
}

std::optional<RemovedLayer> LayerManager::Take(Id id) {
  const int index = IndexOf(id);
  if (index < 0) {
    return std::nullopt;
  }
  RemovedLayer removed{.layer = layers_[static_cast<size_t>(index)],
                       .index = index};
  layers_.erase(layers_.begin() + index);
  Q_EMIT ListChanged();
  return removed;
}

bool LayerManager::Move(Id id, int index) {
  const int from = IndexOf(id);
  if (from < 0) {
    return false;
  }
  const int to = std::clamp(index, 0, Count() - 1);
  if (to == from) {
    return true;
  }
  const Layer layer = layers_[static_cast<size_t>(from)];
  layers_.erase(layers_.begin() + from);
  layers_.insert(layers_.begin() + to, layer);
  Q_EMIT ListChanged();
  return true;
}

void LayerManager::Clear() {
  layers_.clear();
  Q_EMIT ListChanged();
}

bool LayerManager::SetName(Id id, const QString& name) {
  Layer* layer = FindMutable(id);
  if (layer == nullptr) {
    return false;
  }
  layer->name = name;
  Q_EMIT LayerChanged(id);
  return true;
}

bool LayerManager::SetVisible(Id id, bool visible) {
  Layer* layer = FindMutable(id);
  if (layer == nullptr) {
    return false;
  }
  layer->visible = visible;
  Q_EMIT LayerChanged(id);
  return true;
}

bool LayerManager::SetLocked(Id id, bool locked) {
  Layer* layer = FindMutable(id);
  if (layer == nullptr) {
    return false;
  }
  layer->locked = locked;
  Q_EMIT LayerChanged(id);
  return true;
}

const Layer* LayerManager::Find(Id id) const {
  const int index = IndexOf(id);
  return index < 0 ? nullptr : &layers_[static_cast<size_t>(index)];
}

Layer* LayerManager::FindMutable(Id id) {
  const int index = IndexOf(id);
  return index < 0 ? nullptr : &layers_[static_cast<size_t>(index)];
}

// A linear search is fine: there are at most kMaxLayers.
int LayerManager::IndexOf(Id id) const {
  for (size_t i = 0; i < layers_.size(); ++i) {
    if (layers_[i].id == id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace snapper
