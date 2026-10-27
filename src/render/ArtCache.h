#pragma once

#include <QElapsedTimer>
#include <QImage>
#include <QTransform>

#include <vector>

#include "render/OnionSkin.h"

class QPainter;
class QSize;

namespace snapper {

class Document;
struct Pose;

// Keeps the page and artwork drawn for one view, so pen strokes, hovering
// and overlays redraw without drawing every line again. When drawing the
// art is slow, a moving view shows the last drawing moved along with it,
// and the art is drawn again once the view comes to rest.
class ArtCache {
 public:
  // Drawing slower than this counts as slow: a frame at 60 per second.
  static constexpr qint64 kFrameBudgetMs = 16;

  // The artwork changed and must be drawn again.
  void Invalidate() { has_art_ = false; }

  // Draws the art for the view with canvas to screen `matrix`. With
  // `may_reuse`, slow art drawn for another view is moved to fit instead.
  // Returns true when what it drew was reused, so the caller can ask for a
  // sharp drawing later.
  // `onion` frames are drawn faded under the art.
  bool Draw(QPainter& painter, const Document& document, const Pose& pose,
            const std::vector<OnionFrame>& onion, const QTransform& matrix,
            const QSize& size, qreal pixel_ratio, bool may_reuse);

 private:
  void Render(const Document& document, const Pose& pose,
              const std::vector<OnionFrame>& onion, const QTransform& matrix,
              const QSize& size, qreal pixel_ratio);

  QImage image_;
  QTransform matrix_;
  bool has_art_ = false;
  qint64 last_render_ms_ = 0;
};

}  // namespace snapper
