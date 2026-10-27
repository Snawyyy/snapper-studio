#include "eval/FrameEvaluator.h"

#include <array>

#include "document/Document.h"
#include "edit/Playhead.h"
#include "eval/Curves.h"

namespace snapper {

namespace {

QPointF Read(const ChannelManager& channels, Id target,
             ChannelProperty property, const QPointF& setup, int frame) {
  const Channel* channel = channels.Find(target, property);
  if (channel == nullptr) {
    return setup;
  }
  // Each channel of a target shakes differently.
  const Id seed = target * kChannelPropertyCount + static_cast<Id>(property);
  return Curves::ValueAt(channel->keys, setup, frame, seed);
}

}  // namespace

FrameEvaluator::FrameEvaluator(const Document& document,
                               const Playhead& playhead, QObject* parent)
    : QObject(parent), document_(document), playhead_(playhead) {
  const auto changed = [this] { DocumentChanged(); };
  connect(&document_.Layers(), &LayerManager::ListChanged, this, changed);
  connect(&document_.Layers(), &LayerManager::LayerChanged, this, changed);
  connect(&document_.Shapes(), &ShapeManager::LayerContentChanged, this,
          changed);
  connect(&document_.Channels(), &ChannelManager::ChannelChanged, this,
          changed);
  connect(&document_.Skeletons(), &SkeletonManager::BonesChanged, this,
          changed);
  connect(&document_.Bindings(), &BindingManager::LayerBindingChanged, this,
          changed);
  connect(&document_, &Document::Reset, this, changed);
  connect(&playhead_, &Playhead::FrameChanged, this,
          [this] { Q_EMIT PoseChanged(); });
}

LayerTransform FrameEvaluator::TransformAt(const Document& document,
                                           const Layer& layer, int frame) {
  const ChannelManager& channels = document.Channels();
  const LayerTransform& setup = layer.transform;
  LayerTransform transform;
  transform.position = Read(channels, layer.id, ChannelProperty::kLayerPosition,
                            setup.position, frame);
  transform.angle = Read(channels, layer.id, ChannelProperty::kLayerAngle,
                         QPointF(setup.angle, 0), frame)
                        .x();
  transform.scale = Read(channels, layer.id, ChannelProperty::kLayerScale,
                         setup.scale, frame);
  return transform;
}

BoneTransform FrameEvaluator::BoneTransformAt(const Document& document,
                                              const Bone& bone, int frame) {
  const ChannelManager& channels = document.Channels();
  const BoneTransform& setup = bone.transform;
  BoneTransform transform;
  transform.position = Read(channels, bone.id, ChannelProperty::kBonePosition,
                            setup.position, frame);
  transform.angle = Read(channels, bone.id, ChannelProperty::kBoneAngle,
                         QPointF(setup.angle, 0), frame)
                        .x();
  transform.scale = Read(channels, bone.id, ChannelProperty::kBoneScale,
                         QPointF(setup.scale, 0), frame)
                        .x();
  return transform;
}

namespace {

// Bone space to its bone layer's space. A parent's scale stretches it
// along its length, so its children's bases move with its tip.
QTransform BoneMatrix(const BoneTransform& transform, const PosedBone* parent,
                      bool is_setup) {
  if (parent == nullptr) {
    return transform.Matrix();
  }
  const double stretch =
      is_setup ? parent->setup_scale : parent->transform.scale;
  BoneTransform placed = transform;
  placed.position.setX(placed.position.x() * stretch);
  return placed.Matrix() * (is_setup ? parent->setup_matrix : parent->matrix);
}

void PoseBones(const Document& document, int frame,
               const QHash<Id, BoneTransform>& trial, Pose& pose) {
  // Parents come first, so theirs is already posed.
  for (const Bone& bone : document.Skeletons().All()) {
    const PosedBone* parent = pose.FindBone(bone.parent);
    PosedBone posed;
    posed.layer = bone.layer;
    posed.length = bone.length;
    const auto tried = trial.constFind(bone.id);
    posed.transform = tried != trial.cend() ? *tried
                                            : FrameEvaluator::BoneTransformAt(
                                                  document, bone, frame);
    posed.setup_scale = bone.transform.scale;
    posed.matrix = BoneMatrix(posed.transform, parent, false);
    posed.setup_matrix = BoneMatrix(bone.transform, parent, true);
    pose.bones.insert(bone.id, posed);
  }
}

}  // namespace

Pose FrameEvaluator::Evaluate(const Document& document, int frame,
                              const QHash<Id, BoneTransform>& trial) {
  const ChannelManager& channels = document.Channels();
  Pose pose;
  pose.frame = frame;
  PoseBones(document, frame, trial, pose);
  for (const Layer& layer : document.Layers().All()) {
    const LayerTransform transform = TransformAt(document, layer, frame);
    pose.layer_transforms.insert(layer.id, transform);
    // Parents come first, so theirs is already there. A bound layer is
    // carried from where its bone was in the setup pose to where it is,
    // while it sits in that bone's layer.
    const PosedBone* bone =
        pose.FindBone(document.Bindings().BoneOfLayer(layer.id));
    const bool is_following = bone != nullptr && bone->layer == layer.parent;
    const QTransform follow = is_following ? bone->Follow() : QTransform();
    const QTransform outer = follow * pose.LayerMatrix(layer.parent);
    pose.outer_matrices.insert(layer.id, outer);
    pose.layer_matrices.insert(layer.id,
                               transform.Matrix(layer.origin) * outer);
  }

  pose.shapes = document.Shapes().All();
  const bool has_channels = channels.Count() > 0;
  for (size_t s = 0; s < pose.shapes.size(); ++s) {
    Shape& shape = pose.shapes[s];
    pose.shape_index.insert(shape.id, static_cast<int>(s));
    for (size_t p = 0; p < shape.points.size(); ++p) {
      Point& point = shape.points[p];
      pose.point_index.insert(point.id,
                              {static_cast<int>(s), static_cast<int>(p)});
      if (has_channels) {
        point.pos = Read(channels, point.id, ChannelProperty::kPointPosition,
                         point.pos, frame);
      }
    }
  }
  return pose;
}

const Pose& FrameEvaluator::At(int frame) {
  const auto found = kept_.find(frame);
  if (found != kept_.end()) {
    return found->second;
  }
  // Make room, never dropping the current pose, which callers hold on to.
  const int current = playhead_.Frame();
  for (auto it = kept_.begin();
       it != kept_.end() && static_cast<int>(kept_.size()) >= kMaxKeptPoses;) {
    it = it->first == current ? std::next(it) : kept_.erase(it);
  }
  const bool is_trial = frame == 0 && !trial_.isEmpty();
  return kept_
      .emplace(frame, Evaluate(document_, frame,
                               is_trial ? trial_ : decltype(trial_){}))
      .first->second;
}

void FrameEvaluator::SetTrial(QHash<Id, BoneTransform> trial) {
  const bool is_same = trial.isEmpty() && trial_.isEmpty();
  if (is_same) {
    return;
  }
  trial_ = std::move(trial);
  kept_.erase(0);
  Q_EMIT PoseChanged();
}

const Pose& FrameEvaluator::Current() { return At(playhead_.Frame()); }

void FrameEvaluator::DocumentChanged() {
  kept_.clear();
  Q_EMIT PoseChanged();
}

}  // namespace snapper
