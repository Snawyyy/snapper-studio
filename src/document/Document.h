#pragma once

#include <QColor>
#include <QObject>
#include <QSize>

#include "document/BindingManager.h"
#include "document/BrushManager.h"
#include "document/ChannelManager.h"
#include "document/Ids.h"
#include "document/ImageStore.h"
#include "document/LayerManager.h"
#include "document/ShapeManager.h"
#include "document/SkeletonManager.h"
#include "document/StyleManager.h"

namespace snapper {

struct CanvasSettings {
  static constexpr int kMaxSide = 16384;

  QSize size{1920, 1080};
  int fps = 24;
  int frames = 240;
  QColor background = Qt::white;
};

// Everything a project holds, as plain lists, for loading in one go.
struct DocumentContents {
  CanvasSettings canvas;
  Id next_id = 1;
  std::vector<Layer> layers;
  std::vector<Shape> shapes;
  std::vector<Style> styles;
  std::vector<Brush> brushes;
  std::vector<Bone> bones;
  std::vector<LayerBinding> layer_bindings;
  std::vector<Channel> channels;
  // Picture file bytes by name, for image layers and brushes.
  QHash<QString, QByteArray> images;
};

// The whole project: the data managers plus canvas settings. Commands are
// the only thing allowed to change it.
class Document : public QObject {
  Q_OBJECT

 public:
  explicit Document(QObject* parent = nullptr);

  // Empties everything and starts a project ready to draw: one vector
  // layer and the standard brushes. Like Moho, it has no styles until the
  // user makes one.
  void ResetToNew();
  // Empties everything and fills it from loaded contents. Items the
  // managers refuse (bad ids, over the limits, unreadable pictures) are
  // skipped and counted.
  int Replace(const DocumentContents& contents);
  // Adds the brushes that come with the app, with their pictures. Returns
  // how many were added.
  int AddStandardBrushes();
  // Only the pictures some layer or brush still uses are included, and
  // only the bones, channels and bindings of things that still exist.
  DocumentContents Contents() const;

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
  StyleManager& Styles() { return styles_; }
  const StyleManager& Styles() const { return styles_; }
  BrushManager& Brushes() { return brushes_; }
  const BrushManager& Brushes() const { return brushes_; }
  SkeletonManager& Skeletons() { return skeletons_; }
  const SkeletonManager& Skeletons() const { return skeletons_; }
  BindingManager& Bindings() { return bindings_; }
  const BindingManager& Bindings() const { return bindings_; }
  ChannelManager& Channels() { return channels_; }
  const ChannelManager& Channels() const { return channels_; }
  ImageStore& Images() { return images_; }
  const ImageStore& Images() const { return images_; }

  // True when the channel's target is still in the project.
  bool IsAlive(const Channel& channel) const;
  // True when the layer can follow the bone: both exist and the bone is
  // on the nearest bone layer the layer sits in.
  bool CanBind(Id layer, Id bone) const;

 Q_SIGNALS:
  void CanvasChanged();
  // The whole document was replaced, by a new project or a load.
  void Reset();

 private:
  CanvasSettings canvas_;
  Id next_id_ = 1;
  LayerManager layers_;
  ShapeManager shapes_;
  StyleManager styles_;
  BrushManager brushes_;
  SkeletonManager skeletons_;
  BindingManager bindings_;
  ChannelManager channels_;
  ImageStore images_;
};

}  // namespace snapper
