#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

#include <functional>
#include <vector>

namespace snapper {

class CommandRegistry;
class Document;

// Owns the current frame and playback. Frame 0 is the setup pose, and
// playback loops over frames 1 to the last.
class Playhead : public QObject {
  Q_OBJECT

 public:
  // Keys the canvas may use first, like arrows nudging selected points.
  // True when it did.
  using CanvasKey = std::function<bool(int key, Qt::KeyboardModifiers)>;
  // Frames holding keys on the active layer, sorted.
  using KeyFrames = std::function<std::vector<int>()>;

  explicit Playhead(Document& document, QObject* parent = nullptr);

  int Frame() const { return frame_; }
  int LastFrame() const;
  // Clamped to the frames there are.
  void SetFrame(int frame);
  void Step(int frames);

  bool IsPlaying() const { return timer_.isActive(); }
  // Stopping goes back to the frame playback started from.
  void Play();
  void Stop();
  void TogglePlay();

  // Frame stepping, jumping to keys, Home, End and Play.
  void RegisterCommands(CommandRegistry& commands, const CanvasKey& canvas_key,
                        const KeyFrames& key_frames);

 Q_SIGNALS:
  void FrameChanged(int frame);
  void PlayingChanged(bool playing);

 private:
  void Tick();
  void JumpToKey(const std::vector<int>& frames, int direction);

  Document& document_;
  int frame_ = 0;
  int play_from_ = 0;
  QTimer timer_;
  QElapsedTimer clock_;
};

}  // namespace snapper
