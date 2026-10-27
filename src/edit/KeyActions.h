#pragma once

#include <QObject>
#include <QString>

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "document/ChannelManager.h"

namespace snapper {

class CommandRegistry;
class Document;
class FrameEvaluator;
class Playhead;
class SelectionManager;
class UndoManager;

// The timeline's rows for the active layer, Moho style: every key, then
// each kind of channel.
enum class KeyRow : std::uint8_t { kAll, kMove, kRotate, kScale, kPoints };
constexpr int kKeyRowCount = 5;

// A key cell in the timeline: everything in the row on that frame.
struct KeyRef {
  KeyRow row = KeyRow::kAll;
  int frame = 1;

  bool operator==(const KeyRef&) const = default;
};

// Everything you can do to keys on the active layer: add, delete, retime,
// copy and change their curves, plus which keys are selected. Each change
// goes through undo. The Timeline panel and the command list both call in
// here.
class KeyActions : public QObject {
  Q_OBJECT

 public:
  KeyActions(Document& document, SelectionManager& selection, UndoManager& undo,
             Playhead& playhead, FrameEvaluator& frames,
             std::function<void(const QString&)> hint,
             QObject* parent = nullptr);

  static QString RowName(KeyRow row);

  // Frames holding keys in the row, sorted.
  std::vector<int> FramesIn(KeyRow row) const;
  // The curve of the row's keys on the frame, when they all share one.
  std::optional<Curve> CurveAt(KeyRow row, int frame) const;

  const std::vector<KeyRef>& Selected() const { return selected_; }
  bool IsSelected(const KeyRef& key) const;
  void Select(std::vector<KeyRef> keys);
  // The curve the selected keys share, if they do.
  std::optional<Curve> SelectedCurve() const;

  // Keys the selected points on the current frame, or the whole layer
  // when no points are selected.
  void AddKeys();
  // Deletes the selected keys, or with none selected, the keys on the
  // current frame for the selected points or the layer.
  void DeleteKeys();
  // Moves the selected keys along by `frames`, or copies them there.
  void MoveSelected(int frames, bool copy);
  void SetCurve(Curve curve);

  // Add Keyframe, Delete Keyframe and a command per curve.
  void RegisterCommands(CommandRegistry& commands);

 Q_SIGNALS:
  // Keys on the active layer, or which are selected, changed.
  void KeysChanged();

 private:
  // The channels a row covers, as they are now. Missing ones come empty.
  std::vector<Channel> ChannelsIn(KeyRow row) const;
  std::vector<Channel> PointChannels(bool selected_only) const;
  // Applies an edit to each channel the selected keys touch, and pushes
  // the changed ones as one undo step.
  void EditSelected(
      const QString& text,
      const std::function<void(Channel&, const std::vector<int>&)>& edit);
  void Push(const QString& text, std::vector<Channel> before,
            std::vector<Channel> after);

  Document& document_;
  SelectionManager& selection_;
  UndoManager& undo_;
  Playhead& playhead_;
  FrameEvaluator& frames_;
  std::function<void(const QString&)> hint_;
  std::vector<KeyRef> selected_;
};

}  // namespace snapper
