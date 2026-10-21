#include "document/BrushManager.h"

#include <algorithm>

namespace snapper {

Brush Brush::Clamped() const {
  Brush brush = *this;
  brush.spacing = std::clamp(spacing, kMinSpacing, kMaxSpacing);
  brush.angle_jitter = std::clamp(angle_jitter, 0.0F, kMaxAngleJitter);
  brush.spacing_jitter = std::clamp(spacing_jitter, 0.0F, 1.0F);
  brush.size_jitter = std::clamp(size_jitter, 0.0F, 1.0F);
  brush.boil_every = std::clamp(boil_every, 0, kMaxBoilEvery);
  return brush;
}

BrushManager::BrushManager(QObject* parent) : QObject(parent) {}

bool BrushManager::Insert(const Brush& brush, int index) {
  const bool is_full = Count() >= kMaxBrushes;
  const bool is_bad_id = brush.id == kNoId || IndexOf(brush.id) >= 0;
  if (is_full || is_bad_id) {
    return false;
  }
  const int at = std::clamp(index, 0, Count());
  brushes_.insert(brushes_.begin() + at, brush.Clamped());
  Q_EMIT ListChanged();
  return true;
}

std::optional<RemovedBrush> BrushManager::Take(Id id) {
  const int index = IndexOf(id);
  if (index < 0) {
    return std::nullopt;
  }
  RemovedBrush removed{.brush = brushes_[static_cast<size_t>(index)],
                       .index = index};
  brushes_.erase(brushes_.begin() + index);
  Q_EMIT ListChanged();
  return removed;
}

bool BrushManager::Set(const Brush& brush) {
  const int index = IndexOf(brush.id);
  if (index < 0) {
    return false;
  }
  brushes_[static_cast<size_t>(index)] = brush.Clamped();
  Q_EMIT BrushChanged(brush.id);
  return true;
}

void BrushManager::Clear() {
  brushes_.clear();
  Q_EMIT ListChanged();
}

const Brush* BrushManager::Find(Id id) const {
  const int index = IndexOf(id);
  return index < 0 ? nullptr : &brushes_[static_cast<size_t>(index)];
}

int BrushManager::IndexOf(Id id) const {
  if (id == kNoId) {
    return -1;
  }
  const auto found = std::ranges::find(brushes_, id, &Brush::id);
  return found == brushes_.end()
             ? -1
             : static_cast<int>(std::distance(brushes_.begin(), found));
}

}  // namespace snapper
