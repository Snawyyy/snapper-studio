#include "edit/Playhead.h"

#include <QApplication>
#include <QLineEdit>

#include <algorithm>

#include "app/CommandRegistry.h"
#include "document/Document.h"

namespace snapper {

namespace {

// Checks the clock often, so frames land on time at any frame rate.
constexpr int kTickMs = 4;

}  // namespace

Playhead::Playhead(Document& document, QObject* parent)
    : QObject(parent), document_(document) {
  timer_.setInterval(kTickMs);
  timer_.setTimerType(Qt::PreciseTimer);
  connect(&timer_, &QTimer::timeout, this, [this] { Tick(); });
  connect(&document_, &Document::Reset, this, [this] {
    timer_.stop();
    Q_EMIT PlayingChanged(false);
    SetFrame(0);
  });
  connect(&document_, &Document::CanvasChanged, this,
          [this] { SetFrame(frame_); });
}

int Playhead::LastFrame() const { return document_.Canvas().frames; }

void Playhead::SetFrame(int frame) {
  const int clamped = std::clamp(frame, 0, LastFrame());
  if (clamped == frame_) {
    return;
  }
  frame_ = clamped;
  Q_EMIT FrameChanged(frame_);
}

void Playhead::Step(int frames) { SetFrame(frame_ + frames); }

void Playhead::Play() {
  if (IsPlaying()) {
    return;
  }
  play_from_ = frame_;
  // Playing from the setup pose or the end starts at frame 1.
  const bool is_outside_loop = frame_ < 1 || frame_ >= LastFrame();
  if (is_outside_loop) {
    SetFrame(1);
  }
  clock_.start();
  timer_.start();
  Q_EMIT PlayingChanged(true);
}

void Playhead::Stop() {
  if (!IsPlaying()) {
    return;
  }
  timer_.stop();
  SetFrame(play_from_);
  Q_EMIT PlayingChanged(false);
}

void Playhead::TogglePlay() {
  if (IsPlaying()) {
    Stop();
  } else {
    Play();
  }
}

// The frame comes from the clock, not a count of ticks, so slow drawing
// skips frames instead of slowing the animation down.
void Playhead::Tick() {
  const int fps = std::max(1, document_.Canvas().fps);
  const int loop = std::max(1, LastFrame());
  const bool started_inside = play_from_ >= 1 && play_from_ < LastFrame();
  const int start = started_inside ? play_from_ : 1;
  const auto passed = static_cast<int>(clock_.elapsed() * fps / 1000);
  SetFrame(1 + (start - 1 + passed) % loop);
}

void Playhead::JumpToKey(const std::vector<int>& frames, int direction) {
  if (direction < 0) {
    const auto it = std::ranges::lower_bound(frames, frame_);
    SetFrame(it == frames.begin() ? 0 : *std::prev(it));
    return;
  }
  const auto it = std::ranges::upper_bound(frames, frame_);
  if (it != frames.end()) {
    SetFrame(*it);
  }
}

void Playhead::RegisterCommands(CommandRegistry& commands,
                                const CanvasKey& canvas_key,
                                const KeyFrames& key_frames) {
  // Arrows go to the canvas first, which nudges selected points.
  const auto step = [canvas_key](int key, Qt::KeyboardModifiers mods,
                                 const std::function<void()>& action) {
    if (!canvas_key(key, mods)) {
      action();
    }
  };
  commands.Add({
      .id = QStringLiteral("frame.next"),
      .title = QStringLiteral("Next Frame"),
      .shortcut = QKeySequence(Qt::Key_Right),
      .run =
          [this, step] {
            step(Qt::Key_Right, Qt::NoModifier, [this] { Step(1); });
          },
      .alternate = QKeySequence(Qt::Key_Period),
  });
  commands.Add({
      .id = QStringLiteral("frame.previous"),
      .title = QStringLiteral("Previous Frame"),
      .shortcut = QKeySequence(Qt::Key_Left),
      .run =
          [this, step] {
            step(Qt::Key_Left, Qt::NoModifier, [this] { Step(-1); });
          },
      .alternate = QKeySequence(Qt::Key_Comma),
  });
  commands.Add({
      .id = QStringLiteral("frame.next-key"),
      .title = QStringLiteral("Next Keyframe"),
      .shortcut = QKeySequence(Qt::SHIFT | Qt::Key_Right),
      .run =
          [this, step, key_frames] {
            step(Qt::Key_Right, Qt::ShiftModifier,
                 [this, key_frames] { JumpToKey(key_frames(), 1); });
          },
  });
  commands.Add({
      .id = QStringLiteral("frame.previous-key"),
      .title = QStringLiteral("Previous Keyframe"),
      .shortcut = QKeySequence(Qt::SHIFT | Qt::Key_Left),
      .run =
          [this, step, key_frames] {
            step(Qt::Key_Left, Qt::ShiftModifier,
                 [this, key_frames] { JumpToKey(key_frames(), -1); });
          },
  });
  commands.Add({
      .id = QStringLiteral("frame.first"),
      .title = QStringLiteral("Go to Setup Frame"),
      .shortcut = QKeySequence(Qt::Key_Home),
      .run = [this] { SetFrame(0); },
  });
  commands.Add({
      .id = QStringLiteral("frame.last"),
      .title = QStringLiteral("Go to Last Frame"),
      .shortcut = QKeySequence(Qt::Key_End),
      .run = [this] { SetFrame(LastFrame()); },
  });
  commands.Add({
      .id = QStringLiteral("frame.play"),
      .title = QStringLiteral("Play or Stop"),
      .shortcut = QKeySequence(Qt::Key_Return),
      .run =
          [this] {
            // A field being typed in keeps Enter for itself.
            auto* field = qobject_cast<QLineEdit*>(QApplication::focusWidget());
            if (field != nullptr) {
              Q_EMIT field->returnPressed();
              Q_EMIT field->editingFinished();
              return;
            }
            TogglePlay();
          },
      .alternate = QKeySequence(Qt::Key_Enter),
  });
}

}  // namespace snapper
