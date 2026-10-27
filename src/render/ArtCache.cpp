#include "render/ArtCache.h"

#include <QPainter>

#include "render/Renderer.h"

namespace snapper {

bool ArtCache::Draw(QPainter& painter, const Document& document,
                    const Pose& pose, const std::vector<OnionFrame>& onion,
                    const QTransform& matrix, const QSize& size,
                    qreal pixel_ratio, bool may_reuse) {
  const QSize pixels = (size.toSizeF() * pixel_ratio).toSize();
  const bool is_same_view = image_.size() == pixels && matrix_ == matrix;
  const bool is_slow = last_render_ms_ > kFrameBudgetMs;
  const bool can_move_old = has_art_ && may_reuse && is_slow &&
                            image_.size() == pixels && matrix_.isInvertible();
  const bool is_current = has_art_ && is_same_view;
  if (!is_current && can_move_old) {
    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    // Old screen to canvas, then canvas to the new screen.
    painter.setTransform(matrix_.inverted() * matrix);
    painter.drawImage(QPointF(0, 0), image_);
    painter.restore();
    return true;
  }
  if (!is_current) {
    Render(document, pose, onion, matrix, size, pixel_ratio);
  }
  painter.drawImage(QPointF(0, 0), image_);
  return false;
}

void ArtCache::Render(const Document& document, const Pose& pose,
                      const std::vector<OnionFrame>& onion,
                      const QTransform& matrix, const QSize& size,
                      qreal pixel_ratio) {
  QElapsedTimer timer;
  timer.start();
  const QSize pixels = (size.toSizeF() * pixel_ratio).toSize();
  if (image_.size() != pixels) {
    image_ = QImage(pixels, QImage::Format_ARGB32_Premultiplied);
  }
  image_.setDevicePixelRatio(pixel_ratio);
  image_.fill(Qt::transparent);
  {
    QPainter painter(&image_);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setTransform(matrix);
    Renderer::DrawPage(painter, document);
    OnionSkin::Draw(painter, document, onion, matrix, pixels, pixel_ratio);
    Renderer::DrawLayers(painter, document, pose);
  }
  matrix_ = matrix;
  has_art_ = true;
  last_render_ms_ = timer.elapsed();
}

}  // namespace snapper
