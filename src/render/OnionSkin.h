#pragma once

#include <QColor>
#include <QImage>
#include <QTransform>

#include <vector>

class QPainter;

namespace snapper {

class Document;
struct Pose;

// How many frames on each side onion skin shows.
struct OnionSettings {
  static constexpr int kMaxFrames = 10;

  bool on = false;
  int before = 2;
  int after = 2;
};

// A nearby frame to show faded under the current one.
struct OnionFrame {
  const Pose* pose = nullptr;
  // How many frames away, negative for earlier ones.
  int offset = 0;
};

// Draws nearby frames faded and tinted, earlier ones red and later ones
// green, so you see where the motion comes from and goes.
class OnionSkin {
 public:
  // The frames to show around `current`, farthest first. Frame 0 is the
  // setup pose and never shows.
  static std::vector<int> Frames(const OnionSettings& settings, int current,
                                 int last);

  // The painter draws on the art image at `pixels` size. `matrix` is
  // canvas to screen.
  static void Draw(QPainter& painter, const Document& document,
                   const std::vector<OnionFrame>& frames,
                   const QTransform& matrix, const QSize& pixels,
                   qreal pixel_ratio);
};

}  // namespace snapper
