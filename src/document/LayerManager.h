#pragma once

#include <QObject>
#include <QString>

#include <cstdint>
#include <optional>
#include <vector>

#include "document/Ids.h"

namespace snapper {

enum class LayerType : std::uint8_t { kVector, kImage };

struct Layer {
  Id id = kNoId;
  Id parent = kNoId;  // kNoId means top level
  LayerType type = LayerType::kVector;
  QString name{};
  bool visible = true;
  bool locked = false;
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
  void Clear();

  bool SetName(Id id, const QString& name);
  bool SetVisible(Id id, bool visible);
  bool SetLocked(Id id, bool locked);

  const Layer* Find(Id id) const;
  // -1 when no layer has this id.
  int IndexOf(Id id) const;
  int Count() const { return static_cast<int>(layers_.size()); }
  const std::vector<Layer>& All() const { return layers_; }

 Q_SIGNALS:
  // A layer was added, removed or moved.
  void ListChanged();
  // A layer's name, visibility or lock changed.
  void LayerChanged(Id id);

 private:
  Layer* FindMutable(Id id);

  std::vector<Layer> layers_;
};

}  // namespace snapper
