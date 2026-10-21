#pragma once

#include <QWidget>

#include "app/Theme.h"
#include "render/ViewTransform.h"
#include "ui/ViewNavigator.h"

namespace snapper {

class CommandRegistry;
class Document;
class SelectionManager;
class ToolManager;
struct ToolInput;

// The canvas in the middle of the window. Draws the document, hands
// moving around to the view navigator, and every other pointer event, from
// a pen or a mouse, to the tools.
class CanvasView : public QWidget {
  Q_OBJECT

 public:
  CanvasView(Document& document, SelectionManager& selection,
             const Theme& theme, QWidget* parent = nullptr);

  // Fit Canvas, Fit Selection, Actual Size, Zoom In and Zoom Out.
  void RegisterCommands(CommandRegistry& commands);

  void FitCanvas();
  void FitSelection();
  void ZoomAround(double factor);
  void ActualSize();
  const ViewTransform& View() const { return view_; }
  void SetTools(ToolManager* tools);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void tabletEvent(QTabletEvent* event) override;
  void leaveEvent(QEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void enterEvent(QEnterEvent* event) override;
  bool event(QEvent* event) override;

 private:
  void DrawSurround(QPainter& painter) const;
  void UpdateCursor();
  // The user moved the view, so resizing no longer refits it.
  void ViewMoved();
  ToolInput MakeInput(const QPointF& screen, float pressure,
                      Qt::KeyboardModifiers modifiers) const;
  bool StartViewDrag(Qt::MouseButton button, Qt::KeyboardModifiers modifiers,
                     const QPointF& pos);

  Document& document_;
  SelectionManager& selection_;
  const Theme& theme_;
  ViewTransform view_;
  ViewNavigator navigator_{view_};
  bool has_fitted_ = false;
  // True until the user zooms, pans or turns, so resizing keeps the fit.
  bool is_fit_view_ = false;
  ToolManager* tools_ = nullptr;
  bool is_tool_pressed_ = false;
};

}  // namespace snapper
