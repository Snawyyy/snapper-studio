#include "document/LayerManager.h"

#include <algorithm>

namespace snapper {

QTransform LayerTransform::Matrix(const QPointF& origin) const {
  QTransform matrix;
  matrix.translate(position.x() + origin.x(), position.y() + origin.y());
  matrix.rotateRadians(angle);
  matrix.scale(scale.x(), scale.y());
  matrix.translate(-origin.x(), -origin.y());
  return matrix;
}

LayerManager::LayerManager(QObject* parent) : QObject(parent) {}

bool LayerManager::Insert(const Layer& layer, int index) {
  const bool is_full = Count() >= kMaxLayers;
  const bool is_bad_id = layer.id == kNoId || IndexOf(layer.id) >= 0;
  if (is_full || is_bad_id) {
    return false;
  }
  const int at = std::clamp(index, 0, Count());
  if (!FitsAt(layer.parent, at)) {
    return false;
  }
  layers_.insert(layers_.begin() + at, layer);
  Q_EMIT ListChanged();
  return true;
}

std::optional<RemovedLayer> LayerManager::Take(Id id) {
  const int index = IndexOf(id);
  const bool is_single = index >= 0 && EndOf(id) == index + 1;
  if (!is_single) {
    return std::nullopt;
  }
  RemovedLayer removed{.layer = layers_[static_cast<size_t>(index)],
                       .index = index};
  layers_.erase(layers_.begin() + index);
  Q_EMIT ListChanged();
  return removed;
}

std::vector<RemovedLayer> LayerManager::TakeTree(Id id) {
  const int from = IndexOf(id);
  if (from < 0) {
    return {};
  }
  const int end = EndOf(id);
  std::vector<RemovedLayer> removed;
  for (int i = from; i < end; ++i) {
    removed.push_back({.layer = layers_[static_cast<size_t>(i)], .index = i});
  }
  layers_.erase(layers_.begin() + from, layers_.begin() + end);
  Q_EMIT ListChanged();
  return removed;
}

bool LayerManager::Place(Id id, Id parent, int index) {
  const int from = IndexOf(id);
  const bool is_into_itself = parent == id || IsInside(parent, id);
  if (from < 0 || is_into_itself) {
    return false;
  }
  const int end = EndOf(id);
  int deepest = 0;
  const int depth = DepthOf(id);
  for (int i = from; i < end; ++i) {
    deepest =
        std::max(deepest, DepthOf(layers_[static_cast<size_t>(i)].id) - depth);
  }
  const int new_depth = parent == kNoId ? 0 : DepthOf(parent) + 1;
  if (new_depth + deepest >= kMaxDepth) {
    return false;
  }
  std::vector<Layer> block(layers_.begin() + from, layers_.begin() + end);
  layers_.erase(layers_.begin() + from, layers_.begin() + end);
  const int at = std::clamp(index, 0, Count());
  const bool fits = FitsAt(parent, at);
  const int put_at = fits ? at : from;
  if (fits) {
    block.front().parent = parent;
  }
  layers_.insert(layers_.begin() + put_at, block.begin(), block.end());
  if (fits) {
    Q_EMIT ListChanged();
  }
  return fits;
}

bool LayerManager::Move(Id id, int index) {
  const Layer* layer = Find(id);
  return layer != nullptr && Place(id, layer->parent, index);
}

std::optional<int> LayerManager::StepIndex(Id id, int steps) const {
  const Layer* layer = Find(id);
  if (layer == nullptr || steps == 0) {
    return std::nullopt;
  }
  // The layers beside it, bottom to top, by list index.
  std::vector<int> siblings;
  int place = -1;
  for (int i = 0; i < Count(); ++i) {
    const Layer& other = layers_[static_cast<size_t>(i)];
    if (other.parent != layer->parent) {
      continue;
    }
    if (other.id == id) {
      place = static_cast<int>(siblings.size());
    }
    siblings.push_back(i);
  }
  const int target = place + steps;
  const bool is_past_the_ends =
      target < 0 || target >= static_cast<int>(siblings.size());
  if (is_past_the_ends) {
    return std::nullopt;
  }
  const int size = EndOf(id) - IndexOf(id);
  const Id passed =
      layers_[static_cast<size_t>(siblings[static_cast<size_t>(target)])].id;
  // Indexes below the layer don't shift when it is taken out.
  return steps > 0 ? EndOf(passed) - size : IndexOf(passed);
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

bool LayerManager::SetTransform(Id id, const LayerTransform& transform,
                                const QPointF& origin) {
  Layer* layer = FindMutable(id);
  if (layer == nullptr) {
    return false;
  }
  layer->transform = transform;
  layer->origin = origin;
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

int LayerManager::EndOf(Id id) const {
  const int index = IndexOf(id);
  if (index < 0) {
    return -1;
  }
  int end = index + 1;
  while (end < Count() && IsInside(layers_[static_cast<size_t>(end)].id, id)) {
    ++end;
  }
  return end;
}

bool LayerManager::IsInside(Id layer, Id holder) const {
  const Layer* found = Find(layer);
  for (int depth = 0; found != nullptr && depth < kMaxDepth; ++depth) {
    if (found->parent == holder) {
      return holder != kNoId;
    }
    found = Find(found->parent);
  }
  return false;
}

int LayerManager::DepthOf(Id id) const {
  const Layer* found = Find(id);
  int depth = 0;
  while (found != nullptr && found->parent != kNoId && depth < kMaxDepth) {
    found = Find(found->parent);
    ++depth;
  }
  return depth;
}

bool LayerManager::IsShown(Id id) const {
  const Layer* found = Find(id);
  for (int depth = 0; found != nullptr && depth <= kMaxDepth; ++depth) {
    if (!found->visible) {
      return false;
    }
    found = Find(found->parent);
  }
  return Find(id) != nullptr;
}

bool LayerManager::IsLocked(Id id) const {
  const Layer* found = Find(id);
  for (int depth = 0; found != nullptr && depth <= kMaxDepth; ++depth) {
    if (found->locked) {
      return true;
    }
    found = Find(found->parent);
  }
  return false;
}

Id LayerManager::BoneLayerOf(Id id) const {
  const Layer* found = Find(id);
  for (int depth = 0; found != nullptr && depth <= kMaxDepth; ++depth) {
    if (found->type == LayerType::kBone) {
      return found->id;
    }
    found = Find(found->parent);
  }
  return kNoId;
}

std::vector<int> LayerManager::TopFirst() const {
  // A stack of list indexes still to show. Pushing a group bottom first
  // pops it top first, and each layer pushes the layers inside it.
  std::vector<int> order;
  std::vector<int> stack;
  for (int i = 0; i < Count(); ++i) {
    if (layers_[static_cast<size_t>(i)].parent == kNoId) {
      stack.push_back(i);
    }
  }
  while (!stack.empty()) {
    const int index = stack.back();
    stack.pop_back();
    order.push_back(index);
    const Id id = layers_[static_cast<size_t>(index)].id;
    const int end = EndOf(id);
    for (int i = index + 1; i < end; ++i) {
      if (layers_[static_cast<size_t>(i)].parent == id) {
        stack.push_back(i);
      }
    }
  }
  return order;
}

bool LayerManager::FitsAt(Id parent, int index) const {
  if (parent != kNoId) {
    const Layer* holder = Find(parent);
    const bool can_hold = holder != nullptr && HoldsLayers(holder->type) &&
                          DepthOf(parent) + 1 < kMaxDepth;
    if (!can_hold) {
      return false;
    }
    // The layer below must be the parent or sit inside it.
    const Id below =
        index > 0 ? layers_[static_cast<size_t>(index - 1)].id : kNoId;
    const bool is_in_parent =
        below != kNoId && (below == parent || IsInside(below, parent));
    if (!is_in_parent) {
      return false;
    }
  }
  // The layer above must not be inside a layer this one is outside of,
  // which would split that layer's block.
  if (index >= Count()) {
    return true;
  }
  const Id above_parent = layers_[static_cast<size_t>(index)].parent;
  const bool is_above_in_reach = above_parent == kNoId ||
                                 above_parent == parent ||
                                 IsInside(parent, above_parent);
  return is_above_in_reach;
}

}  // namespace snapper
