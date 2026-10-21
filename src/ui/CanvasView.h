#pragma once

#include <QWidget>

#include <cstdint>

#include "app/Theme.h"
#include "render/ViewTransform.h"

namespace snapper {

class CommandRegistry;
class Document;

// The canvas in the middle of the window. Draws the document and lets you
// move around it freely: the wheel or a pinch zooms toward the pointer,
// Space+drag or the middle button pans, Shift+Space+drag turns the view.
class CanvasView : public QWidget {
  Q_OBJECT

 public:
  CanvasView(Document& document, const Theme& theme, QWidget* parent = nullptr);

  // Fit Canvas, Fit Selection, Actual Size, Zoom In and Zoom Out.
  void RegisterCommands(CommandRegistry& commands);

  void FitCanvas();
  void ZoomAround(double factor);
  void ActualSize();
  const ViewTransform& View() const { return view_; }

 protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void enterEvent(QEnterEvent* event) override;
  bool event(QEvent* event) override;

 private:
  enum class Drag : std::uint8_t { kNone, kPan, kRotate };

  void DrawSurround(QPainter& painter) const;
  void UpdateCursor();
  void HandleGesture(QEvent* event);

  Document& document_;
  const Theme& theme_;
  ViewTransform view_;
  bool has_fitted_ = false;
  bool space_held_ = false;
  Drag drag_ = Drag::kNone;
  QPointF last_drag_pos_;
};

}  // namespace snapper
