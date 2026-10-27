#pragma once

#include <QPointF>

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include "document/ChannelManager.h"
#include "document/LayerManager.h"
#include "document/ShapeManager.h"

class QUndoCommand;

namespace snapper {

class Document;
class Playhead;

// Turns edits on the canvas into changes, Moho style: on frame 0 they
// change the setup pose, on any other frame they set keys there.
class AnimationEditor {
 public:
  static constexpr std::array<ChannelProperty, 3> kLayerProperties = {
      ChannelProperty::kLayerPosition, ChannelProperty::kLayerAngle,
      ChannelProperty::kLayerScale};

  AnimationEditor(Document& document, const Playhead& playhead);

  bool IsSetupFrame() const;

  // Points to new spots in layer space. Null when none of them exist.
  std::unique_ptr<QUndoCommand> MovePoints(
      const std::vector<PointPosition>& from,
      const std::vector<PointPosition>& to, int drag_tag) const;

  // A layer to a new transform on the current frame. A new origin, if
  // given, replaces the layer's own. Null when the layer is missing.
  std::unique_ptr<QUndoCommand> PlaceLayer(Id layer,
                                           const LayerTransform& transform,
                                           std::optional<QPointF> origin,
                                           int drag_tag) const;

  // The channel with a key on the frame holding the value. A key already
  // there keeps its curve.
  static Channel WithKey(Channel channel, int frame, const QPointF& value);
  // A layer transform property as a channel value.
  static QPointF ValueOf(const LayerTransform& transform,
                         ChannelProperty property);

 private:
  Document& document_;
  const Playhead& playhead_;
};

}  // namespace snapper
