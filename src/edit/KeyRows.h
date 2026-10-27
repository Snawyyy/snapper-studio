#pragma once

#include <QString>

#include <cstdint>
#include <vector>

#include "document/ChannelManager.h"

namespace snapper {

class Document;
class SelectionManager;

// The timeline's rows for the active layer, Moho style: every key, then
// each kind of channel. Bone layers swap the points row for their bones'.
enum class KeyRow : std::uint8_t {
  kAll,
  kMove,
  kRotate,
  kScale,
  kPoints,
  kBoneMove,
  kBoneRotate,
  kBoneScale,
};
// The most rows any layer shows.
constexpr int kMaxKeyRows = 7;

// A key cell in the timeline: everything in the row on that frame.
struct KeyRef {
  KeyRow row = KeyRow::kAll;
  int frame = 1;

  bool operator==(const KeyRef&) const = default;
};

// Knows which rows the active layer shows and which channels each covers.
class KeyRows {
 public:
  KeyRows(const Document& document, const SelectionManager& selection);

  static QString Name(KeyRow row);
  static KeyRow RowOf(ChannelProperty property);

  // Top first.
  std::vector<KeyRow> Rows() const;
  // The channels a row covers, as they are now. Missing layer and bone
  // channels come empty, missing point channels are left out.
  std::vector<Channel> ChannelsIn(KeyRow row) const;
  std::vector<Channel> PointChannels(bool selected_only) const;
  // Of the selected bones, or of every bone on the active bone layer when
  // none are selected. Empty for other layers.
  std::vector<Channel> BoneChannels() const;
  // True when the active layer is a bone layer.
  bool IsBoneLayer() const;

 private:
  const Document& document_;
  const SelectionManager& selection_;
};

}  // namespace snapper
