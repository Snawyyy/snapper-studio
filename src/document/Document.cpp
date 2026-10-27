#include "document/Document.h"

#include <algorithm>

#include "document/StandardBrushes.h"

namespace snapper {

Document::Document(QObject* parent) : QObject(parent) {}

void Document::ResetToNew() {
  channels_.Clear();
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
                            .channels = {},
                            .images = {}};
  // Deleting a layer or points leaves their channels behind, for undo.
  for (const Channel& channel : channels_.All()) {
    const bool is_point = channel.property == ChannelProperty::kPointPosition;
    const bool is_alive = is_point
                              ? shapes_.ShapeOfPoint(channel.target) != kNoId
                              : layers_.Find(channel.target) != nullptr;
    if (is_alive) {
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

void Document::SetNextId(Id next) { next_id_ = std::max(next_id_, next); }

void Document::SetCanvas(const CanvasSettings& canvas) {
  canvas_ = canvas;
  Q_EMIT CanvasChanged();
}

}  // namespace snapper
