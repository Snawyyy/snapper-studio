#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>

#include <vector>

#include "document/BindingManager.h"
#include "document/Ids.h"
#include "document/JsonWarnings.h"
#include "document/SkeletonManager.h"

namespace snapper {

// Turns bones and bindings into their project.json parts and back. See
// FILE_FORMAT.md.
class BoneJson {
 public:
  static QJsonObject WriteBone(const Bone& bone);
  static QJsonArray WriteBindings(const std::vector<LayerBinding>& bindings);

  // The usable bones, in file order: each on a layer in `bone_layers`, with
  // its parent earlier on the same layer, and an id not in `taken`. Read
  // ids join `taken` and raise `highest`.
  static std::vector<Bone> ReadBones(const QJsonArray& list,
                                     const QSet<Id>& bone_layers,
                                     QSet<Id>& taken, Id& highest,
                                     JsonWarnings& warnings);
  // The bindings whose layer and bone were both read. Whether the layer
  // sits in the bone's layer is the document's to check.
  static std::vector<LayerBinding> ReadBindings(const QJsonArray& list,
                                                const QSet<Id>& layers,
                                                const QSet<Id>& bones,
                                                JsonWarnings& warnings);
};

}  // namespace snapper
