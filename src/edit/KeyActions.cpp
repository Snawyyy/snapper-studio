#include "edit/KeyActions.h"

#include <QSet>

#include <algorithm>

#include "app/CommandRegistry.h"
#include "document/Document.h"
#include "edit/AnimationEditor.h"
#include "edit/KeyCommands.h"
#include "edit/Playhead.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"
#include "eval/Curves.h"
#include "eval/FrameEvaluator.h"

namespace snapper {

namespace {

// How far a noisy key shakes, by default, in each kind of value.
constexpr double kNoisyMove = 4.0;
constexpr double kNoisyAngle = 0.05;
constexpr double kNoisyScale = 0.03;

double NoisyAmount(ChannelProperty property) {
  switch (property) {
    case ChannelProperty::kLayerAngle:
    case ChannelProperty::kBoneAngle:
      return kNoisyAngle;
    case ChannelProperty::kLayerScale:
    case ChannelProperty::kBoneScale:
      return kNoisyScale;
    default:
      return kNoisyMove;
  }
}

bool HasKeyOn(const Channel& channel, int frame) {
  return std::ranges::find(channel.keys, frame, &Key::frame) !=
         channel.keys.end();
}

}  // namespace

KeyActions::KeyActions(Document& document, SelectionManager& selection,
                       UndoManager& undo, Playhead& playhead,
                       FrameEvaluator& frames,
                       std::function<void(const QString&)> hint,
                       QObject* parent)
    : QObject(parent),
      document_(document),
      selection_(selection),
      undo_(undo),
      playhead_(playhead),
      frames_(frames),
      hint_(std::move(hint)),
      rows_(document, selection) {
  connect(&document_.Channels(), &ChannelManager::ChannelChanged, this,
          [this] { Q_EMIT KeysChanged(); });
  connect(&document_.Shapes(), &ShapeManager::LayerContentChanged, this,
          [this] { Q_EMIT KeysChanged(); });
  // The rows themselves change with the layer.
  const auto drop_selection = [this] {
    Select({});
    Q_EMIT KeysChanged();
  };
  connect(&selection_, &SelectionManager::ActiveLayerChanged, this,
          drop_selection);
  // The bone rows show the selected bones' keys.
  connect(&selection_, &SelectionManager::BonesChanged, this,
          [this] { Q_EMIT KeysChanged(); });
  connect(&document_, &Document::Reset, this, drop_selection);
}

std::vector<int> KeyActions::FramesIn(KeyRow row) const {
  QSet<int> frames;
  for (const Channel& channel : rows_.ChannelsIn(row)) {
    for (const Key& key : channel.keys) {
      frames.insert(key.frame);
    }
  }
  std::vector<int> sorted(frames.cbegin(), frames.cend());
  std::ranges::sort(sorted);
  return sorted;
}

std::optional<Curve> KeyActions::CurveAt(KeyRow row, int frame) const {
  std::optional<Curve> shared;
  for (const Channel& channel : rows_.ChannelsIn(row)) {
    const auto key = std::ranges::find(channel.keys, frame, &Key::frame);
    if (key == channel.keys.end()) {
      continue;
    }
    if (shared.has_value() && *shared != key->curve) {
      return std::nullopt;
    }
    shared = key->curve;
  }
  return shared;
}

bool KeyActions::IsSelected(const KeyRef& key) const {
  return std::ranges::find(selected_, key) != selected_.end();
}

void KeyActions::Select(std::vector<KeyRef> keys) {
  if (keys == selected_) {
    return;
  }
  selected_ = std::move(keys);
  Q_EMIT KeysChanged();
}

std::optional<Curve> KeyActions::SelectedCurve() const {
  std::optional<Curve> shared;
  for (const KeyRef& key : selected_) {
    const std::optional<Curve> curve = CurveAt(key.row, key.frame);
    const bool is_mixed =
        !curve.has_value() || (shared.has_value() && *shared != *curve);
    if (is_mixed) {
      return std::nullopt;
    }
    shared = curve;
  }
  return shared;
}

void KeyActions::Push(const QString& text, std::vector<Channel> before,
                      std::vector<Channel> after) {
  if (after.empty()) {
    return;
  }
  undo_.Push(std::make_unique<ReplaceChannelsCommand>(
      document_, text, std::move(before), std::move(after)));
}

void KeyActions::AddKeys() {
  const int frame = playhead_.Frame();
  if (frame == 0) {
    hint_(QStringLiteral(
        "Add Keyframe: frame 0 is the setup pose, go to a later frame"));
    return;
  }
  const Pose& pose = frames_.Current();
  std::vector<Channel> before;
  std::vector<Channel> after;
  if (!selection_.Points().isEmpty()) {
    for (const Id point : selection_.Points()) {
      const std::optional<QPointF> pos = pose.PositionOf(point);
      if (!pos.has_value()) {
        continue;
      }
      Channel channel =
          document_.Channels().Get(point, ChannelProperty::kPointPosition);
      before.push_back(channel);
      after.push_back(
          AnimationEditor::WithKey(std::move(channel), frame, *pos));
    }
    Push(QStringLiteral("Add Keyframe"), std::move(before), std::move(after));
    return;
  }
  const bool has_bones = rows_.IsBoneLayer() && !selection_.Bones().isEmpty();
  if (has_bones) {
    AddBoneKeys(frame);
    return;
  }
  const Id layer = selection_.ActiveLayer();
  if (document_.Layers().Find(layer) == nullptr) {
    return;
  }
  const LayerTransform now = pose.TransformOf(layer);
  for (const ChannelProperty property : AnimationEditor::kLayerProperties) {
    Channel channel = document_.Channels().Get(layer, property);
    before.push_back(channel);
    after.push_back(AnimationEditor::WithKey(
        std::move(channel), frame, AnimationEditor::ValueOf(now, property)));
  }
  Push(QStringLiteral("Add Keyframe"), std::move(before), std::move(after));
}

void KeyActions::AddBoneKeys(int frame) {
  const Pose& pose = frames_.Current();
  std::vector<Channel> before;
  std::vector<Channel> after;
  for (const Id bone : selection_.Bones()) {
    const PosedBone* posed = pose.FindBone(bone);
    if (posed == nullptr) {
      continue;
    }
    for (const ChannelProperty property : AnimationEditor::kBoneProperties) {
      Channel channel = document_.Channels().Get(bone, property);
      before.push_back(channel);
      after.push_back(AnimationEditor::WithKey(
          std::move(channel), frame,
          AnimationEditor::ValueOf(posed->transform, property)));
    }
  }
  Push(QStringLiteral("Add Keyframe"), std::move(before), std::move(after));
}

void KeyActions::DeleteKeys() {
  if (!selected_.empty()) {
    EditSelected(QStringLiteral("Delete Keyframes"),
                 [](Channel& channel, const std::vector<int>& frames) {
                   std::erase_if(channel.keys, [&](const Key& key) {
                     return std::ranges::find(frames, key.frame) !=
                            frames.end();
                   });
                 });
    Select({});
    return;
  }
  const int frame = playhead_.Frame();
  const bool has_points = !selection_.Points().isEmpty();
  const bool has_bones = rows_.IsBoneLayer() && !selection_.Bones().isEmpty();
  std::vector<Channel> channels = has_points  ? rows_.PointChannels(true)
                                  : has_bones ? rows_.BoneChannels()
                                              : rows_.ChannelsIn(KeyRow::kAll);
  std::vector<Channel> before;
  std::vector<Channel> after;
  for (Channel& channel : channels) {
    if (!HasKeyOn(channel, frame)) {
      continue;
    }
    before.push_back(channel);
    std::erase_if(channel.keys,
                  [frame](const Key& key) { return key.frame == frame; });
    after.push_back(std::move(channel));
  }
  if (after.empty()) {
    hint_(QStringLiteral("Delete Keyframe: no keys on this frame"));
    return;
  }
  Push(QStringLiteral("Delete Keyframe"), std::move(before), std::move(after));
}

void KeyActions::MoveSelected(int frames, bool copy) {
  if (selected_.empty() || frames == 0) {
    return;
  }
  const int last = playhead_.LastFrame();
  const auto moved_to = [&](int frame) {
    return std::clamp(frame + frames, 1, last);
  };
  EditSelected(
      copy ? QStringLiteral("Copy Keyframes")
           : QStringLiteral("Move Keyframes"),
      [&](Channel& channel, const std::vector<int>& picked) {
        std::vector<Key> moving;
        for (const Key& key : channel.keys) {
          if (std::ranges::find(picked, key.frame) != picked.end()) {
            moving.push_back(key);
          }
        }
        if (!copy) {
          std::erase_if(channel.keys, [&](const Key& key) {
            return std::ranges::find(picked, key.frame) != picked.end();
          });
        }
        // Moved keys win over keys already where they land.
        for (Key& key : moving) {
          key.frame = moved_to(key.frame);
          std::erase_if(channel.keys,
                        [&](const Key& old) { return old.frame == key.frame; });
        }
        channel.keys.insert(channel.keys.end(), moving.begin(), moving.end());
        std::ranges::sort(channel.keys, {}, &Key::frame);
      });
  std::vector<KeyRef> shifted;
  for (const KeyRef& key : selected_) {
    const KeyRef next{.row = key.row, .frame = moved_to(key.frame)};
    if (std::ranges::find(shifted, next) == shifted.end()) {
      shifted.push_back(next);
    }
  }
  Select(std::move(shifted));
}

void KeyActions::SetCurve(Curve curve) {
  if (selected_.empty()) {
    hint_(QStringLiteral("Set the curve: select keys in the timeline first"));
    return;
  }
  EditSelected(QStringLiteral("Set Curve: %1").arg(Curves::Name(curve)),
               [curve](Channel& channel, const std::vector<int>& frames) {
                 for (Key& key : channel.keys) {
                   if (std::ranges::find(frames, key.frame) == frames.end()) {
                     continue;
                   }
                   key.curve = curve;
                   key.param = curve == Curve::kNoisy
                                   ? NoisyAmount(channel.property)
                                   : 0;
                 }
               });
  Q_EMIT KeysChanged();
}

void KeyActions::EditSelected(
    const QString& text,
    const std::function<void(Channel&, const std::vector<int>&)>& edit) {
  std::vector<Channel> before;
  std::vector<Channel> after;
  for (Channel& channel : rows_.ChannelsIn(KeyRow::kAll)) {
    std::vector<int> frames;
    for (const KeyRef& key : selected_) {
      const bool is_in_row = key.row == KeyRow::kAll ||
                             key.row == KeyRows::RowOf(channel.property);
      if (is_in_row && HasKeyOn(channel, key.frame)) {
        frames.push_back(key.frame);
      }
    }
    if (frames.empty()) {
      continue;
    }
    Channel changed = channel;
    edit(changed, frames);
    if (changed != channel) {
      before.push_back(std::move(channel));
      after.push_back(std::move(changed));
    }
  }
  Push(text, std::move(before), std::move(after));
}

void KeyActions::RegisterCommands(CommandRegistry& commands) {
  commands.Add({
      .id = QStringLiteral("key.add"),
      .title = QStringLiteral("Add Keyframe"),
      .shortcut = QKeySequence(Qt::Key_K),
      .run = [this] { AddKeys(); },
  });
  commands.Add({
      .id = QStringLiteral("key.delete"),
      .title = QStringLiteral("Delete Keyframe"),
      .shortcut = QKeySequence(Qt::SHIFT | Qt::Key_K),
      .run = [this] { DeleteKeys(); },
  });
  for (int i = 0; i < kCurveCount; ++i) {
    const auto curve = static_cast<Curve>(i);
    commands.Add({
        .id = QStringLiteral("key.curve.") + Curves::FileName(curve),
        .title = QStringLiteral("Curve: %1").arg(Curves::Name(curve)),
        .shortcut = {},
        .run = [this, curve] { SetCurve(curve); },
    });
  }
}

}  // namespace snapper
