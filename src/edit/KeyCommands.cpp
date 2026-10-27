#include "edit/KeyCommands.h"

#include <algorithm>

#include "document/Document.h"

namespace snapper {

namespace {

bool IsSameChannel(const Channel& a, const Channel& b) {
  return a.target == b.target && a.property == b.property;
}

}  // namespace

ReplaceChannelsCommand::ReplaceChannelsCommand(Document& document,
                                               const QString& text,
                                               std::vector<Channel> before,
                                               std::vector<Channel> after,
                                               int drag_tag)
    : QUndoCommand(text),
      document_(document),
      before_(std::move(before)),
      after_(std::move(after)),
      drag_tag_(drag_tag) {}

void ReplaceChannelsCommand::redo() {
  for (const Channel& channel : after_) {
    document_.Channels().Put(channel);
  }
}

void ReplaceChannelsCommand::undo() {
  for (const Channel& channel : before_) {
    document_.Channels().Put(channel);
  }
}

// Matching ids make this cast safe: only this class reports kMergeId. A
// later step may touch channels this one didn't, so those join in.
bool ReplaceChannelsCommand::mergeWith(const QUndoCommand* other) {
  const bool is_same_kind = other->id() == kMergeId;
  if (!is_same_kind) {
    return false;
  }
  const auto* next = static_cast<const ReplaceChannelsCommand*>(other);
  const bool is_same_drag = drag_tag_ != 0 && next->drag_tag_ == drag_tag_;
  if (!is_same_drag) {
    return false;
  }
  for (size_t i = 0; i < next->after_.size(); ++i) {
    const Channel& channel = next->after_[i];
    const auto found = std::ranges::find_if(after_, [&](const Channel& mine) {
      return IsSameChannel(mine, channel);
    });
    if (found != after_.end()) {
      *found = channel;
      continue;
    }
    after_.push_back(channel);
    const auto first = std::ranges::find_if(
        next->before_,
        [&](const Channel& c) { return IsSameChannel(c, channel); });
    if (first != next->before_.end()) {
      before_.push_back(*first);
    }
  }
  return true;
}

PlaceLayerCommand::PlaceLayerCommand(Document& document, Id layer,
                                     LayerState before, LayerState after,
                                     int drag_tag)
    : QUndoCommand(QStringLiteral("Transform Layer")),
      document_(document),
      layer_(layer),
      before_(std::move(before)),
      after_(std::move(after)),
      drag_tag_(drag_tag) {}

void PlaceLayerCommand::redo() { Apply(after_); }
void PlaceLayerCommand::undo() { Apply(before_); }

void PlaceLayerCommand::Apply(const LayerState& state) {
  document_.Layers().SetTransform(layer_, state.transform, state.origin);
  for (const Channel& channel : state.channels) {
    document_.Channels().Put(channel);
  }
}

// Matching ids make this cast safe: only this class reports kMergeId.
bool PlaceLayerCommand::mergeWith(const QUndoCommand* other) {
  const bool is_same_kind = other->id() == kMergeId;
  if (!is_same_kind) {
    return false;
  }
  const auto* next = static_cast<const PlaceLayerCommand*>(other);
  const bool is_same_drag =
      next->layer_ == layer_ && next->drag_tag_ == drag_tag_;
  if (!is_same_drag) {
    return false;
  }
  after_ = next->after_;
  return true;
}

}  // namespace snapper
