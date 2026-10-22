#include "document/Document.h"

#include <algorithm>

#include "document/StandardBrushes.h"

namespace snapper {

Document::Document(QObject* parent) : QObject(parent) {}

void Document::ResetToNew() {
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
    if (!layers_.Insert(layer, layers_.Count())) {
      ++skipped;
    }
  }
  for (const Shape& shape : contents.shapes) {
    if (!shapes_.Insert(shape, shapes_.Count())) {
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
                            .images = {}};
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
