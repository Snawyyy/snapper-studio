#include "ui/NumberField.h"

#include <QLineEdit>
#include <QMouseEvent>
#include <QWheelEvent>

#include <cmath>

namespace snapper {

namespace {

// A drag shorter than this is a click that starts typing.
constexpr double kScrubStartPx = 3.0;
constexpr double kPixelsPerStep = 4.0;
constexpr int kBigStep = 10;
constexpr int kWheelUnitsPerNotch = 120;

}  // namespace

NumberField::NumberField(QWidget* parent) : QDoubleSpinBox(parent) {
  setButtonSymbols(QAbstractSpinBox::NoButtons);
  setKeyboardTracking(false);
  setFocusPolicy(Qt::StrongFocus);
  lineEdit()->installEventFilter(this);
  lineEdit()->setCursor(Qt::SizeHorCursor);
}

bool NumberField::eventFilter(QObject* watched, QEvent* event) {
  if (watched != lineEdit()) {
    return QDoubleSpinBox::eventFilter(watched, event);
  }
  const bool is_typing = lineEdit()->hasFocus();
  switch (event->type()) {
    case QEvent::MouseButtonPress: {
      const auto* mouse = static_cast<QMouseEvent*>(event);
      const bool is_scrub_start =
          mouse->button() == Qt::LeftButton && !is_typing;
      if (is_scrub_start) {
        is_pressed_ = true;
        is_scrubbing_ = false;
        press_pos_ = mouse->position();
        press_value_ = value();
        return true;
      }
      break;
    }
    case QEvent::MouseMove: {
      if (!is_pressed_) {
        break;
      }
      const auto* mouse = static_cast<QMouseEvent*>(event);
      const double dx = mouse->position().x() - press_pos_.x();
      if (std::abs(dx) >= kScrubStartPx) {
        is_scrubbing_ = true;
      }
      if (is_scrubbing_) {
        const double steps = std::round(dx / kPixelsPerStep);
        setValue(press_value_ + steps * singleStep());
      }
      return true;
    }
    case QEvent::MouseButtonRelease: {
      if (!is_pressed_) {
        break;
      }
      is_pressed_ = false;
      // A plain click starts typing, with the whole number selected.
      if (!is_scrubbing_) {
        lineEdit()->setFocus(Qt::MouseFocusReason);
        selectAll();
      }
      is_scrubbing_ = false;
      return true;
    }
    default:
      break;
  }
  return QDoubleSpinBox::eventFilter(watched, event);
}

// Works on hover, without clicking first. Some systems turn Shift+wheel
// into a sideways scroll, so either direction counts.
void NumberField::wheelEvent(QWheelEvent* event) {
  const QPoint delta = event->angleDelta();
  const int units = delta.y() != 0 ? delta.y() : delta.x();
  const bool is_big = event->modifiers().testFlag(Qt::ShiftModifier);
  const int notches = units / kWheelUnitsPerNotch;
  const int sign = units > 0 ? 1 : -1;
  const int steps = notches != 0 ? notches : (units != 0 ? sign : 0);
  stepBy(is_big ? steps * kBigStep : steps);
  event->accept();
}

}  // namespace snapper
