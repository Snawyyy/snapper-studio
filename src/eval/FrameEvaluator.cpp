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
  const Id seed = target * 4 + static_cast<Id>(property);
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

Pose FrameEvaluator::Evaluate(const Document& document, int frame) {
  const ChannelManager& channels = document.Channels();
  Pose pose;
  pose.frame = frame;
  for (const Layer& layer : document.Layers().All()) {
    const LayerTransform transform = TransformAt(document, layer, frame);
    pose.layer_transforms.insert(layer.id, transform);
    pose.layer_matrices.insert(layer.id, transform.Matrix(layer.origin));
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
  return kept_.emplace(frame, Evaluate(document_, frame)).first->second;
}

const Pose& FrameEvaluator::Current() { return At(playhead_.Frame()); }

void FrameEvaluator::DocumentChanged() {
  kept_.clear();
  Q_EMIT PoseChanged();
}

}  // namespace snapper
