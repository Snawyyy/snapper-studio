#include "document/Document.h"

#include <algorithm>

namespace snapper {

Document::Document(QObject* parent) : QObject(parent) {}

// Sets the line color and brush, so changing it restyles every line drawn
// with it, while each line keeps the width it was drawn at.
Style Document::DefaultStyle(Id id) {
  Style style{.id = id, .name = QStringLiteral("Ink")};
  style.parts.line = true;
  style.parts.brush = true;
  return style;
}

void Document::ResetToNew() {
  shapes_.Clear();
  layers_.Clear();
  styles_.Clear();
  images_.Clear();
  canvas_ = CanvasSettings{};
  next_id_ = 1;
  layers_.Insert(Layer{.id = NewId(), .name = QStringLiteral("Layer 1")}, 0);
  styles_.Insert(DefaultStyle(NewId()), 0);
  Q_EMIT Reset();
}

int Document::Replace(const DocumentContents& contents) {
  shapes_.Clear();
  layers_.Clear();
  styles_.Clear();
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
                            .images = {}};
  for (const Layer& layer : contents.layers) {
    const bool has_picture =
        layer.type == LayerType::kImage && images_.All().contains(layer.image);
    if (has_picture) {
      contents.images.insert(layer.image,
                             images_.All().value(layer.image).bytes);
    }
  }
  return contents;
}

void Document::SetNextId(Id next) { next_id_ = std::max(next_id_, next); }

void Document::SetCanvas(const CanvasSettings& canvas) {
  canvas_ = canvas;
  Q_EMIT CanvasChanged();
}

}  // namespace snapper
