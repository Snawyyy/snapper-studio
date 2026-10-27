#include "document/Document.h"

#include <algorithm>

#include "document/StandardBrushes.h"

namespace snapper {

Document::Document(QObject* parent) : QObject(parent) {}

void Document::ResetToNew() {
  channels_.Clear();
  bindings_.Clear();
  skeletons_.Clear();
  shapes_.Clear();
  layers_.Clear();
  styles_.Clear();
  brushes_.Clear();
  images_.Clear();
  canvas_ = CanvasSettings{};
  next_id_ = 1;
  layers_.Insert(Layer{.id = NewId(), .name = QStringLiteral("Layer 1")}, 0);
  AddStandardBrushes();
  Q_EMIT Reset();
}

int Document::Replace(const DocumentContents& contents) {
  channels_.Clear();
  bindings_.Clear();
  skeletons_.Clear();
  shapes_.Clear();
  layers_.Clear();
  styles_.Clear();
  brushes_.Clear();
  images_.Clear();
  canvas_ = contents.canvas;
  next_id_ = 1;
  int skipped = 0;
  for (auto it = contents.images.cbegin(); it != contents.images.cend(); ++it) {
    QString error;
    if (!images_.AddNamed(it.key(), it.value(), &error)) {
      ++skipped;
    }
  }
  for (const Style& style : contents.styles) {
    if (!styles_.Insert(style, styles_.Count())) {
      ++skipped;
    }
  }
  for (const Brush& brush : contents.brushes) {
    if (!brushes_.Insert(brush, brushes_.Count())) {
      ++skipped;
    }
  }
  for (const Layer& layer : contents.layers) {
    if (layers_.Insert(layer, layers_.Count())) {
      continue;
    }
    // One that can't sit in its parent goes to the top level instead.
    Layer top_level = layer;
    top_level.parent = kNoId;
    if (!layers_.Insert(top_level, layers_.Count())) {
      ++skipped;
    }
  }
  for (const Shape& shape : contents.shapes) {
    if (!shapes_.Insert(shape, shapes_.Count())) {
      ++skipped;
    }
  }
  for (const Bone& bone : contents.bones) {
    const Layer* layer = layers_.Find(bone.layer);
    const bool is_on_bone_layer =
        layer != nullptr && layer->type == LayerType::kBone;
    const bool is_added =
        is_on_bone_layer && skeletons_.Insert(bone, skeletons_.Count());
    if (!is_added) {
      ++skipped;
    }
  }
  for (const LayerBinding& binding : contents.layer_bindings) {
    const bool is_bound = CanBind(binding.layer, binding.bone) &&
                          bindings_.BindLayer(binding.layer, binding.bone);
    if (!is_bound) {
      ++skipped;
    }
  }
  for (const Channel& channel : contents.channels) {
    if (!channels_.Put(channel)) {
      ++skipped;
    }
  }
  SetNextId(contents.next_id);
  Q_EMIT Reset();
  return skipped;
}

DocumentContents Document::Contents() const {
  DocumentContents contents{.canvas = canvas_,
                            .next_id = next_id_,
                            .layers = layers_.All(),
                            .shapes = shapes_.All(),
                            .styles = styles_.All(),
                            .brushes = brushes_.All(),
                            .bones = {},
                            .layer_bindings = {},
                            .channels = {},
                            .images = {}};
  // Deleting a layer leaves its bones behind, and deleting a layer, bones
  // or points leaves their channels behind, for undo.
  for (const Bone& bone : skeletons_.All()) {
    if (layers_.Find(bone.layer) != nullptr) {
      contents.bones.push_back(bone);
    }
  }
  for (const LayerBinding& binding : bindings_.LayerBindings()) {
    if (CanBind(binding.layer, binding.bone)) {
      contents.layer_bindings.push_back(binding);
    }
  }
  for (const Channel& channel : channels_.All()) {
    if (IsAlive(channel)) {
      contents.channels.push_back(channel);
    }
  }
  const auto add_picture = [&](const QString& name) {
    const auto found = images_.All().constFind(name);
    if (found != images_.All().cend()) {
      contents.images.insert(name, found->bytes);
    }
  };
  for (const Layer& layer : contents.layers) {
    if (layer.type == LayerType::kImage) {
      add_picture(layer.image);
    }
  }
  for (const Brush& brush : contents.brushes) {
    add_picture(brush.image);
  }
  return contents;
}

int Document::AddStandardBrushes() {
  int added = 0;
  for (const StandardBrush& standard : StandardBrushes()) {
    QString error;
    const std::optional<QString> image = images_.Add(standard.png, &error);
    if (!image.has_value()) {
      continue;
    }
    Brush brush = standard.brush;
    brush.id = NewId();
    brush.image = *image;
    if (brushes_.Insert(brush, brushes_.Count())) {
      ++added;
    }
  }
  return added;
}

bool Document::IsAlive(const Channel& channel) const {
  if (channel.property == ChannelProperty::kPointPosition) {
    return shapes_.ShapeOfPoint(channel.target) != kNoId;
  }
  if (IsBoneProperty(channel.property)) {
    const Bone* bone = skeletons_.Find(channel.target);
    return bone != nullptr && layers_.Find(bone->layer) != nullptr;
  }
  return layers_.Find(channel.target) != nullptr;
}

bool Document::CanBind(Id layer, Id bone) const {
  const Layer* found = layers_.Find(layer);
  const Bone* found_bone = skeletons_.Find(bone);
  const bool can_bind = found != nullptr && found_bone != nullptr &&
                        layers_.BoneLayerOf(found->parent) == found_bone->layer;
  return can_bind;
}

void Document::SetNextId(Id next) { next_id_ = std::max(next_id_, next); }

void Document::SetCanvas(const CanvasSettings& canvas) {
  canvas_ = canvas;
  Q_EMIT CanvasChanged();
}

}  // namespace snapper
