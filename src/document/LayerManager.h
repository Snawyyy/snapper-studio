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

enum class LayerType : std::uint8_t { kVector, kImage };

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
  Id parent = kNoId;  // kNoId means top level
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
// drawing is one forward loop. A parent always comes before its children.
class LayerManager : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMaxLayers = 1024;

  explicit LayerManager(QObject* parent = nullptr);

  // Inserts at index (clamped to the list), so 0 is the bottom. Fails on a
  // missing or repeated id, or a full list.
  bool Insert(const Layer& layer, int index);
  std::optional<RemovedLayer> Take(Id id);
  // Moves a layer to a new index (clamped). False when no layer has the id.
  bool Move(Id id, int index);
  void Clear();

  bool SetName(Id id, const QString& name);
  bool SetVisible(Id id, bool visible);
  bool SetLocked(Id id, bool locked);
  bool SetTransform(Id id, const LayerTransform& transform,
                    const QPointF& origin);

  const Layer* Find(Id id) const;
  // -1 when no layer has this id.
  int IndexOf(Id id) const;
  int Count() const { return static_cast<int>(layers_.size()); }
  const std::vector<Layer>& All() const { return layers_; }

 Q_SIGNALS:
  // A layer was added, removed or moved.
  void ListChanged();
  // A layer's name, visibility, lock or transform changed.
  void LayerChanged(Id id);

 private:
  Layer* FindMutable(Id id);

  std::vector<Layer> layers_;
};

}  // namespace snapper
