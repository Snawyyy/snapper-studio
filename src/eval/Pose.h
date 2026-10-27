#pragma once

#include <QHash>
#include <QPointF>
#include <QTransform>

#include <optional>
#include <vector>

#include "document/Ids.h"
#include "document/LayerManager.h"
#include "document/ShapeManager.h"

namespace snapper {

// The document as it stands on one frame: every shape with its points
// where they sit then, in layer space, and where each layer sits.
struct Pose {
  int frame = 0;
  // In drawing order, the same as the shape manager's list.
  std::vector<Shape> shapes;
  QHash<Id, LayerTransform> layer_transforms;
  // Layer space to canvas.
  QHash<Id, QTransform> layer_matrices;
  // The space a layer's own transform sits in to canvas: its parent's
  // space, after any bone it follows.
  QHash<Id, QTransform> outer_matrices;
  QHash<Id, int> shape_index;
  // Each point's shape and place in it.
  QHash<Id, std::pair<int, int>> point_index;

  QTransform LayerMatrix(Id layer) const {
    return layer_matrices.value(layer, QTransform());
  }
  QTransform OuterMatrix(Id layer) const {
    return outer_matrices.value(layer, QTransform());
  }
  LayerTransform TransformOf(Id layer) const {
    return layer_transforms.value(layer, LayerTransform{});
  }
  const Shape* FindShape(Id id) const {
    const auto found = shape_index.constFind(id);
    return found == shape_index.cend() ? nullptr
                                       : &shapes[static_cast<size_t>(*found)];
  }
  // In layer space.
  std::optional<QPointF> PositionOf(Id point) const {
    const auto found = point_index.constFind(point);
    if (found == point_index.cend()) {
      return std::nullopt;
    }
    return shapes[static_cast<size_t>(found->first)]
        .points[static_cast<size_t>(found->second)]
        .pos;
  }
  // On the canvas, after the layer's transform.
  std::optional<QPointF> CanvasPositionOf(Id point) const {
    const auto found = point_index.constFind(point);
    if (found == point_index.cend()) {
      return std::nullopt;
    }
    const Shape& shape = shapes[static_cast<size_t>(found->first)];
    return LayerMatrix(shape.layer)
        .map(shape.points[static_cast<size_t>(found->second)].pos);
  }
};

}  // namespace snapper
