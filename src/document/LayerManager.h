#pragma once

#include <QObject>
#include <QPointF>
#include <QString>
#include <QTransform>

#include <cstdint>
#include <optional>
#include <vector>

#include "document/Ids.h"

namespace snapper {

// Bone layers hold other layers and the bones they follow.
enum class LayerType : std::uint8_t { kVector, kImage, kBone };

// Where a layer sits, turns and scales. On frame 0 it is the layer's own;
// other frames take it from the layer's channels.
struct LayerTransform {
  QPointF position{};
  double angle = 0.0;  // radians
  QPointF scale{1.0, 1.0};

  bool operator==(const LayerTransform&) const = default;
  // Layer space to canvas, turning and scaling round `origin`.
  QTransform Matrix(const QPointF& origin) const;
};

struct Layer {
  Id id = kNoId;
  Id parent = kNoId;  // kNoId means top level, otherwise a bone layer
  LayerType type = LayerType::kVector;
  QString name{};
  bool visible = true;
  bool locked = false;
  // Image layers: the picture's name in the image store, and where its top
  // left corner sits on the canvas at what scale.
  QString image{};
  QPointF image_pos{};
  double image_scale = 1.0;
  // The spot it turns and scales round, in layer space. It never animates.
  QPointF origin{};
  LayerTransform transform{};
};

// A layer taken out of the tree, with where it was, so it can go back.
struct RemovedLayer {
  Layer layer{};
  int index = 0;
};

// Owns the layer tree. Layers are kept in one flat list, bottom to top, so
// drawing is one forward loop. A parent always comes before its children,
// and the layers inside it follow it in one block, bottom to top.
class LayerManager : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMaxLayers = 1024;
  // How deep layers can sit inside each other.
  static constexpr int kMaxDepth = 16;

  explicit LayerManager(QObject* parent = nullptr);

  static bool HoldsLayers(LayerType type) { return type == LayerType::kBone; }

  // Inserts at index (clamped to the list), so 0 is the bottom. Fails on a
  // missing or repeated id, a full list, or a parent that is missing,
  // can't hold layers, or doesn't hold the index.
  bool Insert(const Layer& layer, int index);
  // Fails on a layer that holds others: take those with TakeTree.
  std::optional<RemovedLayer> Take(Id id);
  // Takes the layer and every layer inside it, in list order, so
  // inserting them back in that order restores them.
  std::vector<RemovedLayer> TakeTree(Id id);
  // Moves a layer, with the layers inside it, to sit in `parent` at an
  // index in the list as it stands without them (clamped). False when it
  // can't go there.
  bool Place(Id id, Id parent, int index);
  // Place in the same parent.
  bool Move(Id id, int index);
  // Where Place should put a layer to step it past `steps` of the layers
  // beside it in the same parent, up for positive steps. Empty when it
  // can't move that way.
  std::optional<int> StepIndex(Id id, int steps) const;
  void Clear();

  bool SetName(Id id, const QString& name);
  bool SetVisible(Id id, bool visible);
  bool SetLocked(Id id, bool locked);
  bool SetTransform(Id id, const LayerTransform& transform,
                    const QPointF& origin);

  const Layer* Find(Id id) const;
  // -1 when no layer has this id.
  int IndexOf(Id id) const;
  // Just past the last layer inside it, or IndexOf + 1 when it holds none.
  // -1 when no layer has this id.
  int EndOf(Id id) const;
  // True when `layer` sits somewhere inside `holder`.
  bool IsInside(Id layer, Id holder) const;
  // 0 for top level layers.
  int DepthOf(Id id) const;
  // Shown when it and every layer it sits in are shown. Locked when it or
  // any of them is locked.
  bool IsShown(Id id) const;
  bool IsLocked(Id id) const;
  // The bone layer it is, or the nearest one it sits in. kNoId for none.
  Id BoneLayerOf(Id id) const;
  // List indexes top first, each layer above the layers inside it, as the
  // Layers panel shows them.
  std::vector<int> TopFirst() const;
  int Count() const { return static_cast<int>(layers_.size()); }
  const std::vector<Layer>& All() const { return layers_; }

 Q_SIGNALS:
  // A layer was added, removed or moved.
  void ListChanged();
  // A layer's name, visibility, lock or transform changed.
  void LayerChanged(Id id);

 private:
  Layer* FindMutable(Id id);
  // True when a layer in `parent` can go at the index, in the list as it
  // stands.
  bool FitsAt(Id parent, int index) const;

  std::vector<Layer> layers_;
};

}  // namespace snapper
