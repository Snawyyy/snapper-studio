#include "ui/ColorPopover.h"

#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

#include "edit/ShapeCommands.h"

namespace snapper {

namespace {

constexpr int kWidth = 232;
constexpr int kPlaneHeight = 150;
constexpr int kStripHeight = 16;
constexpr int kEdgeGap = 8;
constexpr double kRadius = 6.0;
constexpr int kCheckerSize = 6;
constexpr double kKnobRadius = 6.0;

// Gray squares behind see-through colors.
void DrawChecker(QPainter& painter, const QRectF& rect) {
  painter.fillRect(rect, QColor(200, 200, 204));
  for (int y = 0; y * kCheckerSize < rect.height(); ++y) {
    for (int x = (y % 2); x * kCheckerSize < rect.width(); x += 2) {
      painter.fillRect(
          QRectF(rect.left() + x * kCheckerSize, rect.top() + y * kCheckerSize,
                 kCheckerSize, kCheckerSize)
              .intersected(rect),
          QColor(150, 150, 156));
    }
  }
}

// A ring that shows on any color.
void DrawKnob(QPainter& painter, const QPointF& at) {
  painter.setBrush(Qt::NoBrush);
  painter.setPen(QPen(QColor(0, 0, 0, 140), 3.0));
  painter.drawEllipse(at, kKnobRadius, kKnobRadius);
  painter.setPen(QPen(Qt::white, 1.5));
  painter.drawEllipse(at, kKnobRadius, kKnobRadius);
}

// Calls `on_drag` with the pointer's share across and down, 0 to 1, while
// it is pressed.
class DragArea : public QWidget {
 public:
  explicit DragArea(QWidget* parent) : QWidget(parent) {
    setCursor(Qt::CrossCursor);
  }
  std::function<void(double, double)> on_drag;

 protected:
  void mousePressEvent(QMouseEvent* event) override { Report(event); }
  void mouseMoveEvent(QMouseEvent* event) override { Report(event); }

 private:
  void Report(QMouseEvent* event) {
    const bool is_pressed = event->buttons().testFlag(Qt::LeftButton);
    if (!is_pressed || !on_drag) {
      return;
    }
    const QPointF pos = event->position();
    on_drag(std::clamp(pos.x() / std::max(1, width()), 0.0, 1.0),
            std::clamp(pos.y() / std::max(1, height()), 0.0, 1.0));
  }
};

}  // namespace

// Saturation across, brightness up, for one hue.
class ColorPlane : public DragArea {
 public:
  using DragArea::DragArea;
  float hue = 0.0F;
  float saturation = 0.0F;
  float value = 0.0F;

 protected:
  void paintEvent(QPaintEvent* /*event*/) override {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF area = rect();
    QPainterPath clip;
    clip.addRoundedRect(area, kRadius, kRadius);
    painter.setClipPath(clip);
    painter.fillRect(area, QColor::fromHsvF(hue, 1.0F, 1.0F));
    QLinearGradient white(area.topLeft(), area.topRight());
    white.setColorAt(0, Qt::white);
    white.setColorAt(1, QColor(255, 255, 255, 0));
    painter.fillRect(area, white);
    QLinearGradient black(area.topLeft(), area.bottomLeft());
    black.setColorAt(0, QColor(0, 0, 0, 0));
    black.setColorAt(1, Qt::black);
    painter.fillRect(area, black);
    painter.setClipping(false);
    DrawKnob(painter, QPointF(saturation * area.width(),
                              (1.0F - value) * area.height()));
  }
};

// A strip of hues, or of opacities of one color.
class ColorStrip : public DragArea {
 public:
  using DragArea::DragArea;
  bool is_alpha = false;
  QColor color;
  float position = 0.0F;

 protected:
  void paintEvent(QPaintEvent* /*event*/) override {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF area = rect();
    QPainterPath clip;
    clip.addRoundedRect(area, area.height() / 2, area.height() / 2);
    painter.setClipPath(clip);
    QLinearGradient gradient(area.topLeft(), area.topRight());
    if (is_alpha) {
      DrawChecker(painter, area);
      QColor clear = color;
      clear.setAlphaF(0.0F);
      QColor solid = color;
      solid.setAlphaF(1.0F);
      gradient.setColorAt(0, clear);
      gradient.setColorAt(1, solid);
    } else {
      constexpr int kStops = 6;
      for (int i = 0; i <= kStops; ++i) {
        const float share = static_cast<float>(i) / kStops;
        gradient.setColorAt(share,
                            QColor::fromHsvF(std::min(share, 0.999F), 1, 1));
      }
    }
    painter.fillRect(area, gradient);
    painter.setClipping(false);
    const double radius = area.height() / 2;
    DrawKnob(painter, QPointF(radius + position * (area.width() - 2 * radius),
                              area.center().y()));
  }
};

ColorPopover::ColorPopover(QWidget* window)
    : QFrame(window),
      plane_(new ColorPlane(this)),
      hue_(new ColorStrip(this)),
      alpha_(new ColorStrip(this)),
      hex_(new QLineEdit(this)) {
  setObjectName(QStringLiteral("color-popover"));
  setAttribute(Qt::WA_StyledBackground);
  setFixedWidth(kWidth);
  plane_->setFixedHeight(kPlaneHeight);
  hue_->setFixedHeight(kStripHeight);
  alpha_->setFixedHeight(kStripHeight);
  alpha_->is_alpha = true;
  hex_->setObjectName(QStringLiteral("color-hex"));
  hex_->setPlaceholderText(QStringLiteral("#RRGGBB or #AARRGGBB"));

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(8);
  layout->addWidget(plane_);
  layout->addWidget(hue_);
  layout->addWidget(alpha_);
  layout->addWidget(hex_);

  plane_->on_drag = [this](double across, double down) {
    SetHsva(hue_value_, static_cast<float>(across),
            static_cast<float>(1.0 - down), alpha_value_);
  };
  hue_->on_drag = [this](double across, double /*down*/) {
    SetHsva(std::min(static_cast<float>(across), 0.999F), saturation_, value_,
            alpha_value_);
  };
  alpha_->on_drag = [this](double across, double /*down*/) {
    SetHsva(hue_value_, saturation_, value_, static_cast<float>(across));
  };
  connect(hex_, &QLineEdit::editingFinished, this, &ColorPopover::ReadHex);
  hide();
}

void ColorPopover::Open(QWidget* anchor, const QColor& color,
                        OnChange on_change) {
  on_change_ = std::move(on_change);
  anchor_ = anchor;
  first_ = color;
  drag_tag_ = MovePointsCommand::NewDragTag();
  const QColor hsv = color.toHsv();
  hue_value_ = std::max(0.0F, hsv.hsvHueF());
  saturation_ = hsv.hsvSaturationF();
  value_ = hsv.valueF();
  alpha_value_ = hsv.alphaF();
  ShowColor();
  adjustSize();
  // Below the anchor, but never past the window's edge.
  const QWidget* host = parentWidget();
  const QPoint below = anchor->mapTo(host, QPoint(0, anchor->height() + 4));
  move(std::clamp(below.x(), kEdgeGap, host->width() - width() - kEdgeGap),
       std::clamp(below.y(), kEdgeGap, host->height() - height() - kEdgeGap));
  show();
  raise();
  qApp->installEventFilter(this);
  setFocus(Qt::PopupFocusReason);
}

void ColorPopover::Close() {
  qApp->removeEventFilter(this);
  on_change_ = nullptr;
  anchor_ = nullptr;
  hide();
}

bool ColorPopover::IsOpenFor(const QWidget* anchor) const {
  return isVisible() && anchor_ == anchor;
}

void ColorPopover::Report(const QColor& color) {
  if (on_change_) {
    on_change_(color, drag_tag_);
  }
}

void ColorPopover::SetHsva(float hue, float saturation, float value,
                           float alpha) {
  hue_value_ = hue;
  saturation_ = saturation;
  value_ = value;
  alpha_value_ = alpha;
  ShowColor();
  Report(QColor::fromHsvF(hue, saturation, value, alpha));
}

void ColorPopover::ShowColor() {
  const QColor color =
      QColor::fromHsvF(hue_value_, saturation_, value_, alpha_value_);
  plane_->hue = hue_value_;
  plane_->saturation = saturation_;
  plane_->value = value_;
  hue_->position = hue_value_;
  alpha_->color = color;
  alpha_->position = alpha_value_;
  const bool is_solid = color.alpha() == 255;
  hex_->setText(
      color.name(is_solid ? QColor::HexRgb : QColor::HexArgb).toUpper());
  plane_->update();
  hue_->update();
  alpha_->update();
}

void ColorPopover::ReadHex() {
  const QColor color = QColor::fromString(hex_->text().trimmed());
  if (!color.isValid()) {
    ShowColor();
    return;
  }
  const QColor hsv = color.toHsv();
  SetHsva(std::max(0.0F, hsv.hsvHueF()), hsv.hsvSaturationF(), hsv.valueF(),
          hsv.alphaF());
}

// A press anywhere outside closes it, keeping the color. Escape, even
// while typing in the hex field, puts the first color back.
bool ColorPopover::eventFilter(QObject* watched, QEvent* event) {
  const bool is_press = event->type() == QEvent::MouseButtonPress;
  if (is_press) {
    const auto* mouse = static_cast<QMouseEvent*>(event);
    const QPoint global = mouse->globalPosition().toPoint();
    // A press on its own swatch is left for the swatch, which toggles it.
    const bool is_on_anchor =
        anchor_ != nullptr &&
        anchor_->rect().contains(anchor_->mapFromGlobal(global));
    const bool is_inside = rect().contains(mapFromGlobal(global));
    if (!is_inside && !is_on_anchor) {
      Close();
    }
  }
  const bool is_escape =
      event->type() == QEvent::KeyPress &&
      static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape && isVisible();
  if (is_escape) {
    Report(first_);
    Close();
    return true;
  }
  return QFrame::eventFilter(watched, event);
}

}  // namespace snapper
