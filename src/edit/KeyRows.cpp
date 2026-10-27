#include "edit/KeyRows.h"

#include "document/Document.h"
#include "edit/AnimationEditor.h"
#include "edit/SelectionManager.h"

namespace snapper {

KeyRows::KeyRows(const Document& document, const SelectionManager& selection)
    : document_(document), selection_(selection) {}

QString KeyRows::Name(KeyRow row) {
  switch (row) {
    case KeyRow::kAll:
      return QStringLiteral("All");
    case KeyRow::kMove:
      return QStringLiteral("Move");
    case KeyRow::kRotate:
      return QStringLiteral("Rotate");
    case KeyRow::kScale:
      return QStringLiteral("Scale");
    case KeyRow::kPoints:
      return QStringLiteral("Points");
    case KeyRow::kBoneMove:
      return QStringLiteral("Bone move");
    case KeyRow::kBoneRotate:
      return QStringLiteral("Bone rotate");
    case KeyRow::kBoneScale:
      return QStringLiteral("Bone scale");
  }
  return {};
}

KeyRow KeyRows::RowOf(ChannelProperty property) {
  switch (property) {
    case ChannelProperty::kLayerPosition:
      return KeyRow::kMove;
    case ChannelProperty::kLayerAngle:
      return KeyRow::kRotate;
    case ChannelProperty::kLayerScale:
      return KeyRow::kScale;
    case ChannelProperty::kBonePosition:
      return KeyRow::kBoneMove;
    case ChannelProperty::kBoneAngle:
      return KeyRow::kBoneRotate;
    case ChannelProperty::kBoneScale:
      return KeyRow::kBoneScale;
    case ChannelProperty::kPointPosition:
      break;
  }
  return KeyRow::kPoints;
}

bool KeyRows::IsBoneLayer() const {
  const Id active = selection_.ActiveLayer();
  return active != kNoId && selection_.BoneLayer() == active;
}

std::vector<KeyRow> KeyRows::Rows() const {
  if (IsBoneLayer()) {
    return {KeyRow::kAll,      KeyRow::kMove,     KeyRow::kRotate,
            KeyRow::kScale,    KeyRow::kBoneMove, KeyRow::kBoneRotate,
            KeyRow::kBoneScale};
  }
  return {KeyRow::kAll, KeyRow::kMove, KeyRow::kRotate, KeyRow::kScale,
          KeyRow::kPoints};
}

std::vector<Channel> KeyRows::PointChannels(bool selected_only) const {
  std::vector<Channel> channels;
  const Id layer = selection_.ActiveLayer();
  for (const Shape& shape : document_.Shapes().All()) {
    if (shape.layer != layer) {
      continue;
    }
    for (const Point& point : shape.points) {
      const bool is_wanted = !selected_only || selection_.IsSelected(point.id);
      const Channel* channel =
          is_wanted ? document_.Channels().Find(point.id,
                                                ChannelProperty::kPointPosition)
                    : nullptr;
      if (channel != nullptr) {
        channels.push_back(*channel);
      }
    }
  }
  return channels;
}

std::vector<Channel> KeyRows::BoneChannels() const {
  std::vector<Channel> channels;
  if (!IsBoneLayer()) {
    return channels;
  }
  const bool has_selected = !selection_.Bones().isEmpty();
  for (const Id bone :
       document_.Skeletons().BonesOn(selection_.ActiveLayer())) {
    if (has_selected && !selection_.IsBoneSelected(bone)) {
      continue;
    }
    for (const ChannelProperty property : AnimationEditor::kBoneProperties) {
      channels.push_back(document_.Channels().Get(bone, property));
    }
  }
  return channels;
}

std::vector<Channel> KeyRows::ChannelsIn(KeyRow row) const {
  const Id layer = selection_.ActiveLayer();
  std::vector<Channel> channels;
  for (const ChannelProperty property : AnimationEditor::kLayerProperties) {
    const bool is_in_row = row == KeyRow::kAll || row == RowOf(property);
    if (is_in_row) {
      channels.push_back(document_.Channels().Get(layer, property));
    }
  }
  for (Channel& channel : BoneChannels()) {
    const bool is_in_row =
        row == KeyRow::kAll || row == RowOf(channel.property);
    if (is_in_row) {
      channels.push_back(std::move(channel));
    }
  }
  const bool has_points = row == KeyRow::kAll || row == KeyRow::kPoints;
  if (has_points) {
    std::vector<Channel> points = PointChannels(false);
    channels.insert(channels.end(), points.begin(), points.end());
  }
  return channels;
}

}  // namespace snapper
