#include "document/BoneJson.h"

#include <QHash>

#include <algorithm>
#include <cmath>

namespace snapper {

namespace {

constexpr double kMaxCoordinate = 1e6;
constexpr double kMaxScale = 1e3;

double Finite(const QJsonValue& value, double fallback, double low,
              double high) {
  const double number = value.toDouble(fallback);
  return std::isfinite(number) ? std::clamp(number, low, high) : fallback;
}

}  // namespace

QJsonObject BoneJson::WriteBone(const Bone& bone) {
  QJsonObject json{{QStringLiteral("id"), bone.id},
                   {QStringLiteral("layer"), bone.layer},
                   {QStringLiteral("parent"), bone.parent},
                   {QStringLiteral("name"), bone.name},
                   {QStringLiteral("x"), bone.transform.position.x()},
                   {QStringLiteral("y"), bone.transform.position.y()},
                   {QStringLiteral("angle"), bone.transform.angle},
                   {QStringLiteral("length"), bone.length}};
  if (bone.transform.scale != 1.0) {
    json.insert(QStringLiteral("scale"), bone.transform.scale);
  }
  if (bone.pinned) {
    json.insert(QStringLiteral("pinned"), true);
  }
  return json;
}

QJsonArray BoneJson::WriteBindings(const std::vector<LayerBinding>& bindings) {
  QJsonArray list;
  for (const LayerBinding& binding : bindings) {
    list.append(QJsonObject{{QStringLiteral("kind"), QStringLiteral("layer")},
                            {QStringLiteral("layer"), binding.layer},
                            {QStringLiteral("bone"), binding.bone}});
  }
  return list;
}

std::vector<Bone> BoneJson::ReadBones(const QJsonArray& list,
                                      const QSet<Id>& bone_layers,
                                      QSet<Id>& taken, Id& highest,
                                      JsonWarnings& warnings) {
  std::vector<Bone> bones;
  // The layer of every bone read so far, to check parents.
  QHash<Id, Id> layer_of;
  for (const QJsonValue value : list) {
    if (static_cast<int>(bones.size()) >= SkeletonManager::kMaxBones) {
      warnings.Add(QStringLiteral("Dropped bones past the limit"));
      break;
    }
    const QJsonObject json = value.toObject();
    Bone bone;
    bone.id = json.value(QStringLiteral("id")).toInt(kNoId);
    bone.layer = json.value(QStringLiteral("layer")).toInt(kNoId);
    bone.parent = json.value(QStringLiteral("parent")).toInt(kNoId);
    const bool is_bad_id = bone.id <= kNoId || taken.contains(bone.id);
    const bool is_lost = !bone_layers.contains(bone.layer);
    if (is_bad_id || is_lost) {
      warnings.Add(QStringLiteral("Dropped bone %1: its id or bone layer is "
                                  "bad")
                       .arg(bone.id));
      continue;
    }
    const bool is_orphan = bone.parent != kNoId &&
                           layer_of.value(bone.parent, kNoId) != bone.layer;
    if (is_orphan) {
      warnings.Add(QStringLiteral("Bone %1 lost its parent, which is missing")
                       .arg(bone.id));
      bone.parent = kNoId;
    }
    bone.name = json.value(QStringLiteral("name")).toString();
    BoneTransform& transform = bone.transform;
    transform.position = QPointF(Finite(json.value(QStringLiteral("x")), 0,
                                        -kMaxCoordinate, kMaxCoordinate),
                                 Finite(json.value(QStringLiteral("y")), 0,
                                        -kMaxCoordinate, kMaxCoordinate));
    transform.angle = Finite(json.value(QStringLiteral("angle")), 0,
                             -kMaxCoordinate, kMaxCoordinate);
    transform.scale =
        Finite(json.value(QStringLiteral("scale")), 1, -kMaxScale, kMaxScale);
    bone.length = Finite(json.value(QStringLiteral("length")), 100,
                         SkeletonManager::kMinLength, kMaxCoordinate);
    bone.pinned = json.value(QStringLiteral("pinned")).toBool(false);
    taken.insert(bone.id);
    layer_of.insert(bone.id, bone.layer);
    highest = std::max(highest, bone.id);
    bones.push_back(bone);
  }
  return bones;
}

std::vector<LayerBinding> BoneJson::ReadBindings(const QJsonArray& list,
                                                 const QSet<Id>& layers,
                                                 const QSet<Id>& bones,
                                                 JsonWarnings& warnings) {
  std::vector<LayerBinding> bindings;
  for (const QJsonValue value : list) {
    const QJsonObject json = value.toObject();
    if (json.value(QStringLiteral("kind")).toString() != u"layer") {
      warnings.Add(
          QStringLiteral("Dropped a binding of a kind this app doesn't know"));
      continue;
    }
    const LayerBinding binding{
        .layer = json.value(QStringLiteral("layer")).toInt(kNoId),
        .bone = json.value(QStringLiteral("bone")).toInt(kNoId)};
    const bool is_whole =
        layers.contains(binding.layer) && bones.contains(binding.bone);
    if (!is_whole) {
      warnings.Add(QStringLiteral("Dropped the binding of layer %1: its "
                                  "bone is missing")
                       .arg(binding.layer));
      continue;
    }
    bindings.push_back(binding);
  }
  return bindings;
}

}  // namespace snapper
