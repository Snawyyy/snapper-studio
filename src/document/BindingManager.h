#pragma once

#include <QHash>
#include <QObject>

#include <vector>

#include "document/Ids.h"

namespace snapper {

// A whole layer following one bone.
struct LayerBinding {
  Id layer = kNoId;
  Id bone = kNoId;

  bool operator==(const LayerBinding&) const = default;
};

// Owns what follows what. For now that is whole layers following a bone;
// points following bones join later.
class BindingManager : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMaxLayerBindings = 1024;

  explicit BindingManager(QObject* parent = nullptr);

  // Binds the layer to the bone, replacing any bone it had. kNoId for the
  // bone unbinds it. Fails on a missing layer id or a full list.
  bool BindLayer(Id layer, Id bone);
  void Clear();

  // kNoId when the layer follows no bone.
  Id BoneOfLayer(Id layer) const { return layer_bones_.value(layer, kNoId); }
  std::vector<Id> LayersOnBone(Id bone) const;
  // Sorted by layer id, so saving is stable.
  std::vector<LayerBinding> LayerBindings() const;

 Q_SIGNALS:
  void LayerBindingChanged(Id layer);

 private:
  QHash<Id, Id> layer_bones_;
};

}  // namespace snapper
