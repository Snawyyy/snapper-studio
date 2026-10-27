#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>

#include <optional>
#include <vector>

#include "document/ChannelManager.h"
#include "document/Ids.h"
#include "document/JsonWarnings.h"
#include "document/LayerManager.h"
#include "document/ShapeManager.h"

namespace snapper {

// Turns channels and layer transforms into their project.json parts and
// back. See FILE_FORMAT.md.
class ChannelJson {
 public:
  static QJsonObject WriteChannel(const Channel& channel);
  // Adds the layer's origin and setup transform, when it has them.
  static void WriteTransform(const Layer& layer, QJsonObject& json);

  // The ids a channel may animate, by kind.
  struct Targets {
    const QSet<Id>& layers;
    const QSet<Id>& bones;
    const QSet<Id>& points;
  };

  // Nothing when the channel can't be used, with a warning. Its target
  // must be among the targets of its property's kind.
  static std::optional<Channel> ReadChannel(const QJsonObject& json,
                                            const Targets& targets,
                                            JsonWarnings& warnings);
  // Every usable channel in the list. Only points some shape kept can be
  // animated.
  static std::vector<Channel> ReadChannels(const QJsonArray& list,
                                           const QSet<Id>& layers,
                                           const QSet<Id>& bones,
                                           const std::vector<Shape>& shapes,
                                           JsonWarnings& warnings);
  static void ReadTransform(const QJsonObject& json, Layer& layer);
};

}  // namespace snapper
