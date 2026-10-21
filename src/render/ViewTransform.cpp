#include "render/ViewTransform.h"

#include <algorithm>
#include <cmath>

namespace snapper {

QTransform ViewTransform::ToScreenMatrix() const {
  QTransform matrix;
  matrix.translate(view_size_.width() / 2, view_size_.height() / 2);
  matrix.rotate(angle_);
  matrix.scale(zoom_, zoom_);
  matrix.translate(-center_.x(), -center_.y());
  return matrix;
}

QPointF ViewTransform::ToScreen(const QPointF& canvas) const {
  return ToScreenMatrix().map(canvas);
}

QPointF ViewTransform::ToCanvas(const QPointF& screen) const {
  return ToScreenMatrix().inverted().map(screen);
}

void ViewTransform::ZoomAt(const QPointF& screen, double factor) {
  const QPointF anchor = ToCanvas(screen);
  zoom_ = std::clamp(zoom_ * factor, kMinZoom, kMaxZoom);
  Pin(anchor, screen);
}

void ViewTransform::RotateAt(const QPointF& screen, double degrees) {
  const QPointF anchor = ToCanvas(screen);
  angle_ = std::remainder(angle_ + degrees, 360.0);
  Pin(anchor, screen);
}

void ViewTransform::PanBy(const QPointF& screen_delta) {
  const QPointF middle(view_size_.width() / 2, view_size_.height() / 2);
  center_ = ToCanvas(middle - screen_delta);
}

void ViewTransform::Fit(const QRectF& canvas_rect, double margin) {
  angle_ = 0.0;
  center_ = canvas_rect.center();
  const double room_x = std::max(1.0, view_size_.width() - 2 * margin);
  const double room_y = std::max(1.0, view_size_.height() - 2 * margin);
  const double fit_x = room_x / std::max(1.0, canvas_rect.width());
  const double fit_y = room_y / std::max(1.0, canvas_rect.height());
  zoom_ = std::clamp(std::min(fit_x, fit_y), kMinZoom, kMaxZoom);
}

void ViewTransform::ResetZoom() {
  const QPointF middle(view_size_.width() / 2, view_size_.height() / 2);
  center_ = ToCanvas(middle);
  zoom_ = 1.0;
  angle_ = 0.0;
}

void ViewTransform::Pin(const QPointF& canvas, const QPointF& screen) {
  PanBy(screen - ToScreen(canvas));
}

}  // namespace snapper
