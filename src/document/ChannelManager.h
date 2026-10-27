#pragma once

#include <QHash>
#include <QObject>
#include <QPointF>

#include <cstdint>
#include <vector>

#include "document/Ids.h"

namespace snapper {

// What a channel animates on its target. Single numbers, like an angle,
// sit in x.
enum class ChannelProperty : std::uint8_t {
  kPointPosition,
  kLayerPosition,
  kLayerAngle,
  kLayerScale,
  kBonePosition,
  kBoneAngle,
  kBoneScale,
};
constexpr int kChannelPropertyCount = 7;

// True for properties held in x alone.
constexpr bool IsSingleValue(ChannelProperty property) {
  return property == ChannelProperty::kLayerAngle ||
         property == ChannelProperty::kBoneAngle ||
         property == ChannelProperty::kBoneScale;
}

constexpr bool IsBoneProperty(ChannelProperty property) {
  return property == ChannelProperty::kBonePosition ||
         property == ChannelProperty::kBoneAngle ||
         property == ChannelProperty::kBoneScale;
}

// How a key moves on to the next one. A key's curve covers only the span
// after it.
enum class Curve : std::uint8_t {
  kSmooth,
  kLinear,
  kStep,
  kEaseIn,
  kEaseOut,
  kEaseInOut,
  kNoisy,
  kCycle,
  kBounce,
  kElastic,
};
constexpr int kCurveCount = 10;

struct Key {
  int frame = 1;
  QPointF value{};
  Curve curve = Curve::kSmooth;
  // Noisy: how far the value shakes. Cycle: how many frames back it loops,
  // 0 for back to the key before.
  double param = 0.0;

  bool operator==(const Key&) const = default;
};

// One animated value. Frame 0 is the setup pose, taken from the target
// itself, so keys sit on frame 1 and later, sorted by frame.
struct Channel {
  Id target = kNoId;
  ChannelProperty property = ChannelProperty::kPointPosition;
  std::vector<Key> keys;

  bool operator==(const Channel&) const = default;
};

// Owns every channel in the project, one per target and property.
class ChannelManager : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMaxChannels = 1000000;
  static constexpr int kMaxKeysPerChannel = 10000;

  explicit ChannelManager(QObject* parent = nullptr);

  // Replaces the channel for its target and property, or adds it. A
  // channel with no keys is removed. Keys are sorted, and ones before
  // frame 1 or on a repeated frame are dropped. Fails on a missing target
  // or past the limits.
  bool Put(Channel channel);
  void Clear();

  const Channel* Find(Id target, ChannelProperty property) const;
  // An empty channel for the target when it has none.
  Channel Get(Id target, ChannelProperty property) const;
  int Count() const { return static_cast<int>(channels_.size()); }
  const std::vector<Channel>& All() const { return channels_; }

 Q_SIGNALS:
  void ChannelChanged(Id target);

 private:
  static quint64 KeyOf(Id target, ChannelProperty property);
  void RebuildIndex();

  std::vector<Channel> channels_;
  QHash<quint64, int> index_;
};

}  // namespace snapper
