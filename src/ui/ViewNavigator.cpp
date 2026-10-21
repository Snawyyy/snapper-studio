#include "ui/ViewNavigator.h"

#include <cmath>
#include <numbers>

namespace snapper {

namespace {

// One wheel notch (120 units) zooms by about 19%.
constexpr double kZoomPerWheelUnit = 1.0015;

}  // namespace

bool ViewNavigator::StartDrag(Qt::MouseButton button,
                              Qt::KeyboardModifiers modifiers,
                              const QPointF& pos) {
  const bool is_middle = button == Qt::MiddleButton;
  const bool is_space_drag = space_held_ && button == Qt::LeftButton;
  const bool is_rotate = is_space_drag && modifiers.testFlag(Qt::ShiftModifier);
  if (is_rotate) {
    drag_ = Drag::kRotate;
  } else if (is_middle || is_space_drag) {
    drag_ = Drag::kPan;
  } else {
    return false;
  }
  last_pos_ = pos;
  return true;
}

void ViewNavigator::DragTo(const QPointF& pos, const QRectF& view_rect) {
  if (drag_ == Drag::kPan) {
    view_.PanBy(pos - last_pos_);
  } else if (drag_ == Drag::kRotate) {
    // Turns by the angle the pointer swept round the middle of the view.
    const QPointF middle = view_rect.center();
    const QPointF from = last_pos_ - middle;
    const QPointF to = pos - middle;
    const double radians =
        std::atan2(to.y(), to.x()) - std::atan2(from.y(), from.x());
    view_.RotateAt(middle, radians * 180.0 / std::numbers::pi);
  }
  last_pos_ = pos;
}

void ViewNavigator::EndDrag() { drag_ = Drag::kNone; }

void ViewNavigator::Wheel(const QPointF& pos, int units) {
  view_.ZoomAt(pos, std::pow(kZoomPerWheelUnit, units));
}

bool ViewNavigator::Gesture(Qt::NativeGestureType type, double value,
                            const QPointF& pos) {
  switch (type) {
    case Qt::ZoomNativeGesture:
      view_.ZoomAt(pos, 1.0 + value);
      return true;
    case Qt::RotateNativeGesture:
      view_.RotateAt(pos, value);
      return true;
    default:
      return false;
  }
}

std::optional<Qt::CursorShape> ViewNavigator::Cursor() const {
  if (drag_ == Drag::kRotate) {
    return Qt::SizeAllCursor;
  }
  if (drag_ == Drag::kPan) {
    return Qt::ClosedHandCursor;
  }
  if (space_held_) {
    return Qt::OpenHandCursor;
  }
  return std::nullopt;
}

}  // namespace snapper
