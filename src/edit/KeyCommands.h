#pragma once

#include <QPointF>
#include <QUndoCommand>

#include <vector>

#include "document/ChannelManager.h"
#include "document/Ids.h"
#include "document/LayerManager.h"

namespace snapper {

class Document;

// Puts whole channels back and forth: `before` on undo, `after` on redo. A
// channel with no keys is one that isn't there. Steps of one drag, sharing
// a tag other than 0, merge into one undo step.
class ReplaceChannelsCommand : public QUndoCommand {
 public:
  static constexpr int kMergeId = 5;

  ReplaceChannelsCommand(Document& document, const QString& text,
                         std::vector<Channel> before,
                         std::vector<Channel> after, int drag_tag = 0);

  void redo() override;
  void undo() override;
  int id() const override { return kMergeId; }
  bool mergeWith(const QUndoCommand* other) override;

 private:
  Document& document_;
  std::vector<Channel> before_;
  std::vector<Channel> after_;
  int drag_tag_;
};

// A layer's setup transform and origin, with its transform channels.
struct LayerState {
  LayerTransform transform{};
  QPointF origin{};
  std::vector<Channel> channels;
};

// Moves, turns or scales a layer: its setup on frame 0, its keys on other
// frames. Steps of one drag merge.
class PlaceLayerCommand : public QUndoCommand {
 public:
  static constexpr int kMergeId = 6;

  PlaceLayerCommand(Document& document, Id layer, LayerState before,
                    LayerState after, int drag_tag);

  void redo() override;
  void undo() override;
  int id() const override { return kMergeId; }
  bool mergeWith(const QUndoCommand* other) override;

 private:
  void Apply(const LayerState& state);

  Document& document_;
  Id layer_;
  LayerState before_;
  LayerState after_;
  int drag_tag_;
};

}  // namespace snapper
