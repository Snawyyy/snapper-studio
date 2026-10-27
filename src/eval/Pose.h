#pragma once

#include <QHash>
#include <QPointF>
#include <QTransform>

#include <cmath>
#include <optional>
#include <vector>

#include "document/Ids.h"
#include "document/LayerManager.h"
#include "document/ShapeManager.h"
#include "document/SkeletonManager.h"

namespace snapper {

// A bone as it stands on one frame, in its bone layer's space.
struct PosedBone {
  Id layer = kNoId;
  BoneTransform transform{};
  double length = 0.0;
  // Bone space to bone layer space, unstretched: the base at 0, the bone
  // along x. The same for the setup pose.
  QTransform matrix{};
  QTransform setup_matrix{};
  double setup_scale = 1.0;

  QPointF Base() const { return matrix.map(QPointF(0, 0)); }
  // The way it points, in radians in bone layer space.
  double Heading() const { return std::atan2(matrix.m12(), matrix.m11()); }
  // A spot in bone layer space as the position a child of this bone
  // would need to have its base there.
  QPointF ChildPosition(const QPointF& spot) const {
    QPointF position = matrix.inverted().map(spot);
    if (std::abs(transform.scale) > 1e-9) {
      position.setX(position.x() / transform.scale);
    }
    return position;
  }
  QPointF Tip() const {
    return matrix.map(QPointF(length * transform.scale, 0));
  }
  // Moves art from where it sits in the setup pose to where it sits now.
  QTransform Follow() const {
    const QTransform setup =
        QTransform::fromScale(setup_scale, 1) * setup_matrix;
    return setup.inverted() * QTransform::fromScale(transform.scale, 1) *
           matrix;
  }
};

// The document as it stands on one frame: every shape with its points
// where they sit then, in layer space, where each layer sits, and every
// bone.
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
  QHash<Id, PosedBone> bones;
  QHash<Id, int> shape_index;
  // Each point's shape and place in it.
  QHash<Id, std::pair<int, int>> point_index;

  QTransform LayerMatrix(Id layer) const {
    return layer_matrices.value(layer, QTransform());
  }
  const PosedBone* FindBone(Id bone) const {
    const auto found = bones.constFind(bone);
    return found == bones.cend() ? nullptr : &*found;
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
