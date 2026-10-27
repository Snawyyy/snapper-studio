#include "render/OnionSkin.h"

#include <QPainter>

#include <algorithm>
#include <cstdlib>

#include "render/Renderer.h"

namespace snapper {

namespace {

// Apple's system red and green.
const QColor kEarlier(255, 69, 58);
const QColor kLater(48, 209, 88);
constexpr double kNearestOpacity = 0.45;
constexpr double kFarthestOpacity = 0.15;

}  // namespace

std::vector<int> OnionSkin::Frames(const OnionSettings& settings, int current,
                                   int last) {
  std::vector<int> frames;
  if (!settings.on) {
    return frames;
  }
  const int reach = std::max(settings.before, settings.after);
  for (int distance = reach; distance >= 1; --distance) {
    const bool has_earlier =
        distance <= settings.before && current - distance >= 1;
    if (has_earlier) {
      frames.push_back(current - distance);
    }
    const bool has_later =
        distance <= settings.after && current + distance <= last;
    if (has_later) {
      frames.push_back(current + distance);
    }
  }
  return frames;
}

void OnionSkin::Draw(QPainter& painter, const Document& document,
                     const std::vector<OnionFrame>& frames,
                     const QTransform& matrix, const QSize& pixels,
                     qreal pixel_ratio) {
  if (frames.empty()) {
    return;
  }
  int farthest = 1;
  for (const OnionFrame& frame : frames) {
    farthest = std::max(farthest, std::abs(frame.offset));
  }
  QImage layer(pixels, QImage::Format_ARGB32_Premultiplied);
  layer.setDevicePixelRatio(pixel_ratio);
  for (const OnionFrame& frame : frames) {
    layer.fill(Qt::transparent);
    {
      QPainter art(&layer);
      art.setRenderHint(QPainter::Antialiasing);
      art.setTransform(matrix);
      Renderer::DrawLayers(art, document, *frame.pose);
      // Keep the shape of the art, in the tint.
      art.resetTransform();
      art.setCompositionMode(QPainter::CompositionMode_SourceIn);
      art.fillRect(QRectF(QPointF(0, 0), pixels.toSizeF() / pixel_ratio),
                   frame.offset < 0 ? kEarlier : kLater);
    }
    const double nearness =
        farthest == 1 ? 1.0
                      : 1.0 - static_cast<double>(std::abs(frame.offset) - 1) /
                                  (farthest - 1);
    painter.save();
    painter.resetTransform();
    painter.setOpacity(kFarthestOpacity +
                       (kNearestOpacity - kFarthestOpacity) * nearness);
    painter.drawImage(QPointF(0, 0), layer);
    painter.restore();
  }
}

}  // namespace snapper
