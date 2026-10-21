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

void Document::SetNextId(Id next) { next_id_ = std::max(next_id_, next); }

void Document::SetCanvas(const CanvasSettings& canvas) {
  canvas_ = canvas;
  Q_EMIT CanvasChanged();
}

}  // namespace snapper
