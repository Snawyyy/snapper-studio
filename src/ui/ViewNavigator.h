#pragma once

#include <QPointF>
#include <QRectF>

#include <cstdint>
#include <optional>

#include "render/ViewTransform.h"

namespace snapper {

// Moving around the canvas by hand: the wheel and pinch zoom toward the
// pointer, Space+drag or the middle button pans, Shift+Space+drag turns.
// Owns the drag in progress and which cursor it wants.
class ViewNavigator {
 public:
  explicit ViewNavigator(ViewTransform& view) : view_(view) {}

  // Starts a pan or turn when Space or the middle button asks for one.
  // Returns false when the press is for the tools instead.
  bool StartDrag(Qt::MouseButton button, Qt::KeyboardModifiers modifiers,
                 const QPointF& pos);
  void DragTo(const QPointF& pos, const QRectF& view_rect);
  void EndDrag();
  bool IsDragging() const { return drag_ != Drag::kNone; }

  void SetSpaceHeld(bool held) { space_held_ = held; }
  bool IsSpaceHeld() const { return space_held_; }

  void Wheel(const QPointF& pos, int units);
  // Trackpad pinch and twist. Returns false for other gestures.
  bool Gesture(Qt::NativeGestureType type, double value, const QPointF& pos);

  // The cursor for the current state, or none for the tool's own.
  std::optional<Qt::CursorShape> Cursor() const;

 private:
  enum class Drag : std::uint8_t { kNone, kPan, kRotate };

  ViewTransform& view_;
  Drag drag_ = Drag::kNone;
  bool space_held_ = false;
  QPointF last_pos_;
};

}  // namespace snapper
