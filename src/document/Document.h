#pragma once

#include <QColor>
#include <QObject>
#include <QSize>

#include "document/Ids.h"
#include "document/LayerManager.h"
#include "document/ShapeManager.h"

namespace snapper {

struct CanvasSettings {
  QSize size{1920, 1080};
  int fps = 24;
  int frames = 240;
  QColor background = Qt::white;
};

// The whole project: the data managers plus canvas settings. Commands are
// the only thing allowed to change it.
class Document : public QObject {
  Q_OBJECT

 public:
  explicit Document(QObject* parent = nullptr);

  // Empties everything and starts a project ready to draw: one vector
  // layer.
  void ResetToNew();

  // A fresh id, never handed out before in this project.
  Id NewId() { return next_id_++; }
  Id NextId() const { return next_id_; }
  // For loading. Never moves backwards, so handed out ids stay unique.
  void SetNextId(Id next);

  const CanvasSettings& Canvas() const { return canvas_; }
  void SetCanvas(const CanvasSettings& canvas);

  LayerManager& Layers() { return layers_; }
  const LayerManager& Layers() const { return layers_; }
  ShapeManager& Shapes() { return shapes_; }
  const ShapeManager& Shapes() const { return shapes_; }

 Q_SIGNALS:
  void CanvasChanged();
  // The whole document was replaced, by a new project or a load.
  void Reset();

 private:
  CanvasSettings canvas_;
  Id next_id_ = 1;
  LayerManager layers_;
  ShapeManager shapes_;
};

}  // namespace snapper
