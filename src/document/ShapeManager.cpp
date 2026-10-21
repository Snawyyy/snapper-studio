#include "document/ShapeManager.h"

#include <QSet>

#include <algorithm>

namespace snapper {

ShapeManager::ShapeManager(QObject* parent) : QObject(parent) {}

bool ShapeManager::Insert(const Shape& shape, int index) {
  const int point_count = static_cast<int>(shape.points.size());
  const bool is_full = Count() >= kMaxShapes ||
                       PointCount() + point_count > kMaxPointsPerProject;
  const bool is_bad_size = point_count < 2 || point_count > kMaxPointsPerShape;
  const bool is_bad_id = shape.id == kNoId || shape_index_.contains(shape.id);
  if (is_full || is_bad_size || is_bad_id) {
    return false;
  }
  for (const Point& point : shape.points) {
    const bool is_bad_point =
        point.id == kNoId || point_index_.contains(point.id);
    if (is_bad_point) {
      return false;
    }
  }
  const int at = std::clamp(index, 0, Count());
  shapes_.insert(shapes_.begin() + at, shape);
  RebuildIndex();
  Q_EMIT LayerContentChanged(shape.layer);
  return true;
}

std::optional<RemovedShape> ShapeManager::Take(Id id) {
  const int index = IndexOf(id);
  if (index < 0) {
    return std::nullopt;
  }
  RemovedShape removed{.shape = shapes_[static_cast<size_t>(index)],
                       .index = index};
  shapes_.erase(shapes_.begin() + index);
  RebuildIndex();
  Q_EMIT LayerContentChanged(removed.shape.layer);
  return removed;
}

void ShapeManager::Clear() {
  QSet<Id> layers;
  for (const Shape& shape : shapes_) {
    layers.insert(shape.layer);
  }
  shapes_.clear();
  RebuildIndex();
  for (const Id layer : layers) {
    Q_EMIT LayerContentChanged(layer);
  }
}

int ShapeManager::SetPositions(const std::vector<PointPosition>& positions) {
  QSet<Id> layers;
  int moved = 0;
  for (const PointPosition& position : positions) {
    const auto found = point_index_.constFind(position.point);
    if (found == point_index_.cend()) {
      continue;
    }
    Shape& shape = shapes_[static_cast<size_t>(found->shape)];
    shape.points[static_cast<size_t>(found->point)].pos = position.pos;
    layers.insert(shape.layer);
    ++moved;
  }
  for (const Id layer : layers) {
    Q_EMIT LayerContentChanged(layer);
  }
  return moved;
}

const Shape* ShapeManager::Find(Id id) const {
  const int index = IndexOf(id);
  return index < 0 ? nullptr : &shapes_[static_cast<size_t>(index)];
}

int ShapeManager::IndexOf(Id id) const { return shape_index_.value(id, -1); }

Id ShapeManager::ShapeOfPoint(Id point) const {
  const auto found = point_index_.constFind(point);
  if (found == point_index_.cend()) {
    return kNoId;
  }
  return shapes_[static_cast<size_t>(found->shape)].id;
}

std::optional<QPointF> ShapeManager::PositionOf(Id point) const {
  const auto found = point_index_.constFind(point);
  if (found == point_index_.cend()) {
    return std::nullopt;
  }
  const Shape& shape = shapes_[static_cast<size_t>(found->shape)];
  return shape.points[static_cast<size_t>(found->point)].pos;
}

std::vector<Id> ShapeManager::ShapesOn(Id layer) const {
  std::vector<Id> ids;
  for (const Shape& shape : shapes_) {
    if (shape.layer == layer) {
      ids.push_back(shape.id);
    }
  }
  return ids;
}

void ShapeManager::RebuildIndex() {
  shape_index_.clear();
  point_index_.clear();
  for (size_t s = 0; s < shapes_.size(); ++s) {
    const Shape& shape = shapes_[s];
    shape_index_.insert(shape.id, static_cast<int>(s));
    for (size_t p = 0; p < shape.points.size(); ++p) {
      point_index_.insert(
          shape.points[p].id,
          PointRef{.shape = static_cast<int>(s), .point = static_cast<int>(p)});
    }
  }
}

}  // namespace snapper
