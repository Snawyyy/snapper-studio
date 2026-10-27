#include "edit/AnimationEditor.h"

#include <algorithm>

#include "document/Document.h"
#include "edit/BoneCommands.h"
#include "edit/KeyCommands.h"
#include "edit/Playhead.h"
#include "edit/ShapeCommands.h"
#include "eval/FrameEvaluator.h"

namespace snapper {

AnimationEditor::AnimationEditor(Document& document, const Playhead& playhead)
    : document_(document), playhead_(playhead) {}

bool AnimationEditor::IsSetupFrame() const { return playhead_.Frame() == 0; }

std::unique_ptr<QUndoCommand> AnimationEditor::MovePoints(
    const std::vector<PointPosition>& from,
    const std::vector<PointPosition>& to, int drag_tag) const {
  if (to.empty()) {
    return nullptr;
  }
  if (IsSetupFrame()) {
    return std::make_unique<MovePointsCommand>(document_, from, to, drag_tag);
  }
  const int frame = playhead_.Frame();
  std::vector<Channel> before;
  std::vector<Channel> after;
  for (const PointPosition& position : to) {
    if (document_.Shapes().ShapeOfPoint(position.point) == kNoId) {
      continue;
    }
    Channel channel = document_.Channels().Get(position.point,
                                               ChannelProperty::kPointPosition);
    before.push_back(channel);
    after.push_back(WithKey(std::move(channel), frame, position.pos));
  }
  if (after.empty()) {
    return nullptr;
  }
  return std::make_unique<ReplaceChannelsCommand>(
      document_, QStringLiteral("Move Points"), std::move(before),
      std::move(after), drag_tag);
}

std::unique_ptr<QUndoCommand> AnimationEditor::PlaceLayer(
    Id layer, const LayerTransform& transform, std::optional<QPointF> origin,
    int drag_tag) const {
  const Layer* found = document_.Layers().Find(layer);
  if (found == nullptr) {
    return nullptr;
  }
  LayerState before{
      .transform = found->transform, .origin = found->origin, .channels = {}};
  for (const ChannelProperty property : kLayerProperties) {
    before.channels.push_back(document_.Channels().Get(layer, property));
  }
  LayerState after = before;
  after.origin = origin.value_or(found->origin);
  if (IsSetupFrame()) {
    after.transform = transform;
  } else {
    // Only what changed gets a key, so turning a layer leaves its move
    // and scale rows alone.
    const int frame = playhead_.Frame();
    const LayerTransform now =
        FrameEvaluator::TransformAt(document_, *found, frame);
    for (Channel& channel : after.channels) {
      const QPointF value = ValueOf(transform, channel.property);
      const bool is_changed = value != ValueOf(now, channel.property);
      const bool has_key = std::ranges::find(channel.keys, frame,
                                             &Key::frame) != channel.keys.end();
      if (is_changed || has_key) {
        channel = WithKey(std::move(channel), frame, value);
      }
    }
  }
  return std::make_unique<PlaceLayerCommand>(
      document_, layer, std::move(before), std::move(after), drag_tag);
}

std::unique_ptr<QUndoCommand> AnimationEditor::PlaceBones(
    const std::vector<BonePlacement>& to, int drag_tag) const {
  const SkeletonManager& skeletons = document_.Skeletons();
  if (IsSetupFrame()) {
    std::vector<Bone> before;
    std::vector<Bone> after;
    for (const BonePlacement& placement : to) {
      const Bone* bone = skeletons.Find(placement.bone);
      if (bone == nullptr) {
        continue;
      }
      before.push_back(*bone);
      after.push_back(*bone);
      after.back().transform = placement.transform;
    }
    if (after.empty()) {
      return nullptr;
    }
    return std::make_unique<SetBonesCommand>(
        document_, QStringLiteral("Transform Bones"), std::move(before),
        std::move(after), drag_tag);
  }
  // Only what changed gets a key, as for layers.
  const int frame = playhead_.Frame();
  std::vector<Channel> before;
  std::vector<Channel> after;
  for (const BonePlacement& placement : to) {
    const Bone* bone = skeletons.Find(placement.bone);
    if (bone == nullptr) {
      continue;
    }
    const BoneTransform now =
        FrameEvaluator::BoneTransformAt(document_, *bone, frame);
    for (const ChannelProperty property : kBoneProperties) {
      Channel channel = document_.Channels().Get(bone->id, property);
      const QPointF value = ValueOf(placement.transform, property);
      const bool is_changed = value != ValueOf(now, property);
      const bool has_key = std::ranges::find(channel.keys, frame,
                                             &Key::frame) != channel.keys.end();
      if (!is_changed && !has_key) {
        continue;
      }
      before.push_back(channel);
      after.push_back(WithKey(std::move(channel), frame, value));
    }
  }
  if (after.empty()) {
    return nullptr;
  }
  return std::make_unique<ReplaceChannelsCommand>(
      document_, QStringLiteral("Pose Bones"), std::move(before),
      std::move(after), drag_tag);
}

Channel AnimationEditor::WithKey(Channel channel, int frame,
                                 const QPointF& value) {
  const auto found = std::ranges::find(channel.keys, frame, &Key::frame);
  if (found != channel.keys.end()) {
    found->value = value;
    return channel;
  }
  const auto after =
      std::ranges::upper_bound(channel.keys, frame, {}, &Key::frame);
  channel.keys.insert(after, {.frame = frame, .value = value});
  return channel;
}

QPointF AnimationEditor::ValueOf(const BoneTransform& transform,
                                 ChannelProperty property) {
  switch (property) {
    case ChannelProperty::kBoneAngle:
      return {transform.angle, 0};
    case ChannelProperty::kBoneScale:
      return {transform.scale, 0};
    default:
      return transform.position;
  }
}

QPointF AnimationEditor::ValueOf(const LayerTransform& transform,
                                 ChannelProperty property) {
  switch (property) {
    case ChannelProperty::kLayerAngle:
      return {transform.angle, 0};
    case ChannelProperty::kLayerScale:
      return transform.scale;
    default:
      return transform.position;
  }
}

}  // namespace snapper
