#pragma once

#include <QObject>
#include <QString>

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "document/ChannelManager.h"
#include "edit/KeyRows.h"

namespace snapper {

class CommandRegistry;
class Document;
class FrameEvaluator;
class Playhead;
class SelectionManager;
class UndoManager;

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

  // The rows the active layer shows, top first.
  std::vector<KeyRow> Rows() const { return rows_.Rows(); }

  // Frames holding keys in the row, sorted.
  std::vector<int> FramesIn(KeyRow row) const;
  // The curve of the row's keys on the frame, when they all share one.
  std::optional<Curve> CurveAt(KeyRow row, int frame) const;

  const std::vector<KeyRef>& Selected() const { return selected_; }
  bool IsSelected(const KeyRef& key) const;
  void Select(std::vector<KeyRef> keys);
  // The curve the selected keys share, if they do.
  std::optional<Curve> SelectedCurve() const;

  // Keys the selected points or bones on the current frame, or the whole
  // layer when none are selected.
  void AddKeys();
  // Deletes the selected keys, or with none selected, the keys on the
  // current frame for the selected points or bones, or the layer.
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
  // Keys of the selected bones on the current frame, as posed now.
  void AddBoneKeys(int frame);
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
  KeyRows rows_;
  std::vector<KeyRef> selected_;
};

}  // namespace snapper
