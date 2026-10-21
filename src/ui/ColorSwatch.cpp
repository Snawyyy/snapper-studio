#include "ui/ColorSwatch.h"

#include <QPainter>
#include <QPainterPath>

#include "ui/ColorPopover.h"

namespace snapper {

namespace {

constexpr int kWidth = 44;
constexpr int kHeight = 24;
constexpr double kRadius = 6.0;

}  // namespace

ColorSwatch::ColorSwatch(ColorPopover& popover, QWidget* parent)
    : QAbstractButton(parent), popover_(popover) {
  setCursor(Qt::PointingHandCursor);
  connect(this, &QAbstractButton::clicked, this, [this] {
    if (popover_.IsOpenFor(this)) {
      popover_.Close();
      return;
    }
    popover_.Open(this, color_, [this](const QColor& color, int drag_tag) {
      SetColor(color);
      Q_EMIT ColorChanged(color, drag_tag);
    });
  });
}

void ColorSwatch::SetColor(const QColor& color) {
  if (color != color_) {
    color_ = color;
    setToolTip(
        color.name(color.alpha() == 255 ? QColor::HexRgb : QColor::HexArgb)
            .toUpper());
    update();
  }
}

QSize ColorSwatch::sizeHint() const { return {kWidth, kHeight}; }

// A see-through color shows over a light and a dark half.
void ColorSwatch::paintEvent(QPaintEvent* /*event*/) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const QRectF area = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
  QPainterPath shape;
  shape.addRoundedRect(area, kRadius, kRadius);
  painter.setClipPath(shape);
  if (color_.alpha() < 255) {
    painter.fillRect(area, QColor(200, 200, 204));
    painter.fillRect(
        QRectF(area.left(), area.top(), area.width() / 2, area.height()),
        QColor(90, 90, 96));
  }
  painter.fillRect(area, color_);
  painter.setClipping(false);
  const QColor edge = isEnabled() ? palette().color(QPalette::Light)
                                  : palette().color(QPalette::Dark);
  painter.setPen(QPen(edge, 1.0));
  painter.setBrush(Qt::NoBrush);
  painter.drawPath(shape);
}

}  // namespace snapper
