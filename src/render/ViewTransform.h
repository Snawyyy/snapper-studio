#pragma once

#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QTransform>

namespace snapper {

// Where the canvas sits in the view: which canvas point is at the middle
// of the view, how far it is zoomed and how far it is turned. Screen
// points are view pixels with (0, 0) at the top left.
class ViewTransform {
 public:
  static constexpr double kMinZoom = 0.02;
  static constexpr double kMaxZoom = 64.0;

  void SetViewSize(const QSizeF& size) { view_size_ = size; }
  const QSizeF& ViewSize() const { return view_size_; }

  double Zoom() const { return zoom_; }
  // In degrees, clockwise.
  double Angle() const { return angle_; }
  QPointF Center() const { return center_; }

  QTransform ToScreenMatrix() const;
  QPointF ToScreen(const QPointF& canvas) const;
  QPointF ToCanvas(const QPointF& screen) const;

  // Zooms by a factor, keeping the canvas point under `screen` in place.
  void ZoomAt(const QPointF& screen, double factor);
  // Turns by degrees around a screen point, keeping it in place.
  void RotateAt(const QPointF& screen, double degrees);
  void PanBy(const QPointF& screen_delta);

  // Shows the whole rect in the view with a margin, upright.
  void Fit(const QRectF& canvas_rect, double margin);
  // 100% zoom and upright, keeping the middle of the view in place.
  void ResetZoom();

 private:
  // Moves the center so the canvas point lands on the screen point.
  void Pin(const QPointF& canvas, const QPointF& screen);

  QSizeF view_size_{1, 1};
  QPointF center_;
  double zoom_ = 1.0;
  double angle_ = 0.0;
};

}  // namespace snapper
