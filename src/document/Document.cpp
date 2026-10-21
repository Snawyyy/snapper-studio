#include "document/Document.h"

#include <algorithm>

namespace snapper {

Document::Document(QObject* parent) : QObject(parent) {}

void Document::ResetToNew() {
  shapes_.Clear();
  layers_.Clear();
  canvas_ = CanvasSettings{};
  next_id_ = 1;
  layers_.Insert(Layer{.id = NewId(), .name = QStringLiteral("Layer 1")}, 0);
  Q_EMIT Reset();
}

int Document::Replace(const DocumentContents& contents) {
  shapes_.Clear();
  layers_.Clear();
  canvas_ = contents.canvas;
  next_id_ = 1;
  int skipped = 0;
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
  return {.canvas = canvas_,
          .next_id = next_id_,
          .layers = layers_.All(),
          .shapes = shapes_.All()};
}

void Document::SetNextId(Id next) { next_id_ = std::max(next_id_, next); }

void Document::SetCanvas(const CanvasSettings& canvas) {
  canvas_ = canvas;
  Q_EMIT CanvasChanged();
}

}  // namespace snapper
