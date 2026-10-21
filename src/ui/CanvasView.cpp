#include "ui/CanvasView.h"

#include <QApplication>
#include <QLineEdit>
#include <QNativeGestureEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <cmath>
#include <numbers>

#include "app/CommandRegistry.h"
#include "document/Document.h"
#include "render/Renderer.h"

namespace snapper {

namespace {

constexpr double kFitMargin = 48.0;
// One wheel notch (120 units) zooms by about 19%.
constexpr double kZoomPerWheelUnit = 1.0015;
constexpr double kZoomStep = 1.25;
// The area around the page covers artwork drawn off the page, faded.
constexpr int kOffPageCoverAlpha = 170;
constexpr int kShadowLayers = 4;
constexpr int kShadowAlpha = 18;

}  // namespace

CanvasView::CanvasView(Document& document, const Theme& theme, QWidget* parent)
    : QWidget(parent), document_(document), theme_(theme) {
  setObjectName(QStringLiteral("canvas"));
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setAttribute(Qt::WA_AcceptTouchEvents);

  const auto repaint = [this] { update(); };
  connect(&document_.Layers(), &LayerManager::ListChanged, this, repaint);
  connect(&document_.Layers(), &LayerManager::LayerChanged, this, repaint);
  connect(&document_.Shapes(), &ShapeManager::LayerContentChanged, this,
          repaint);
  connect(&document_, &Document::CanvasChanged, this, repaint);
  connect(&document_, &Document::Reset, this, [this] { FitCanvas(); });
}

void CanvasView::RegisterCommands(CommandRegistry& commands) {
  commands.Add({
      .id = QStringLiteral("view.fit-canvas"),
      .title = QStringLiteral("Fit Canvas"),
      .shortcut = QKeySequence(Qt::SHIFT | Qt::Key_F),
      .run = [this] { FitCanvas(); },
  });
  // Until there is a selection to fit, this fits the canvas.
  commands.Add({
      .id = QStringLiteral("view.fit-selection"),
      .title = QStringLiteral("Fit Selection"),
      .shortcut = QKeySequence(Qt::Key_F),
      .run = [this] { FitCanvas(); },
  });
  commands.Add({
      .id = QStringLiteral("view.actual-size"),
      .title = QStringLiteral("Actual Size"),
      .shortcut = QKeySequence(Qt::Key_1),
      .run = [this] { ActualSize(); },
  });
  commands.Add({
      .id = QStringLiteral("view.zoom-in"),
      .title = QStringLiteral("Zoom In"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::Key_Equal),
      .run = [this] { ZoomAround(kZoomStep); },
  });
  commands.Add({
      .id = QStringLiteral("view.zoom-out"),
      .title = QStringLiteral("Zoom Out"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::Key_Minus),
      .run = [this] { ZoomAround(1 / kZoomStep); },
  });
}

void CanvasView::FitCanvas() {
  const QSizeF size = document_.Canvas().size.toSizeF();
  view_.Fit(QRectF(QPointF(0, 0), size), kFitMargin);
  update();
}

void CanvasView::ZoomAround(double factor) {
  view_.ZoomAt(rect().center(), factor);
  update();
}

void CanvasView::ActualSize() {
  view_.ResetZoom();
  update();
}

void CanvasView::paintEvent(QPaintEvent* /*event*/) {
  QPainter painter(this);
  painter.fillRect(rect(), theme_.canvas_surround);
  painter.setRenderHint(QPainter::Antialiasing);

  painter.setTransform(view_.ToScreenMatrix());
  Renderer::DrawPage(painter, document_);
  Renderer::DrawLayers(painter, document_);
  painter.resetTransform();
  DrawSurround(painter);
}

// Fades artwork that lies off the page and draws a soft shadow round it.
void CanvasView::DrawSurround(QPainter& painter) const {
  const QSizeF size = document_.Canvas().size.toSizeF();
  const QPolygonF page =
      view_.ToScreenMatrix().map(QPolygonF(QRectF(QPointF(0, 0), size)));
  QPainterPath outside;
  outside.addRect(rect());
  QPainterPath page_path;
  page_path.addPolygon(page);
  page_path.closeSubpath();

  QColor cover = theme_.canvas_surround;
  cover.setAlpha(kOffPageCoverAlpha);
  const QPainterPath off_page = outside.subtracted(page_path);
  painter.fillPath(off_page, cover);

  // The shadow falls only outside the page.
  painter.setClipPath(off_page);
  painter.setBrush(Qt::NoBrush);
  for (int i = 1; i <= kShadowLayers; ++i) {
    painter.setPen(QPen(QColor(0, 0, 0, kShadowAlpha), i * 2.0));
    painter.drawPolygon(page.translated(0, i * 0.5));
  }
}

void CanvasView::resizeEvent(QResizeEvent* event) {
  view_.SetViewSize(size().toSizeF());
  QWidget::resizeEvent(event);
}

// The first time the view has its real size, the page fills it.
void CanvasView::showEvent(QShowEvent* event) {
  view_.SetViewSize(size().toSizeF());
  if (!has_fitted_) {
    has_fitted_ = true;
    FitCanvas();
  }
  QWidget::showEvent(event);
}

void CanvasView::wheelEvent(QWheelEvent* event) {
  const double units = event->angleDelta().y();
  view_.ZoomAt(event->position(), std::pow(kZoomPerWheelUnit, units));
  update();
  event->accept();
}

void CanvasView::mousePressEvent(QMouseEvent* event) {
  const bool is_middle = event->button() == Qt::MiddleButton;
  const bool is_space_drag = space_held_ && event->button() == Qt::LeftButton;
  const bool is_rotate =
      is_space_drag && event->modifiers().testFlag(Qt::ShiftModifier);
  if (is_rotate) {
    drag_ = Drag::kRotate;
  } else if (is_middle || is_space_drag) {
    drag_ = Drag::kPan;
  }
  last_drag_pos_ = event->position();
  UpdateCursor();
}

void CanvasView::mouseMoveEvent(QMouseEvent* event) {
  const QPointF pos = event->position();
  if (drag_ == Drag::kPan) {
    view_.PanBy(pos - last_drag_pos_);
    update();
  } else if (drag_ == Drag::kRotate) {
    // Turns by the angle the pointer swept round the middle of the view.
    const QPointF middle = rect().toRectF().center();
    const QPointF from = last_drag_pos_ - middle;
    const QPointF to = pos - middle;
    const double radians =
        std::atan2(to.y(), to.x()) - std::atan2(from.y(), from.x());
    view_.RotateAt(middle, radians * 180.0 / std::numbers::pi);
    update();
  }
  last_drag_pos_ = pos;
}

void CanvasView::mouseReleaseEvent(QMouseEvent* /*event*/) {
  drag_ = Drag::kNone;
  UpdateCursor();
}

void CanvasView::keyPressEvent(QKeyEvent* event) {
  const bool is_space = event->key() == Qt::Key_Space;
  if (!is_space) {
    QWidget::keyPressEvent(event);
    return;
  }
  if (!event->isAutoRepeat()) {
    space_held_ = true;
    UpdateCursor();
  }
  event->accept();
}

void CanvasView::keyReleaseEvent(QKeyEvent* event) {
  const bool is_space = event->key() == Qt::Key_Space;
  if (!is_space) {
    QWidget::keyReleaseEvent(event);
    return;
  }
  if (!event->isAutoRepeat()) {
    space_held_ = false;
    UpdateCursor();
  }
  event->accept();
}

// Keys like Space reach the canvas while the pointer is over it, unless
// the user is typing into a field.
void CanvasView::enterEvent(QEnterEvent* event) {
  const bool is_typing =
      qobject_cast<QLineEdit*>(QApplication::focusWidget()) != nullptr;
  if (!is_typing) {
    setFocus(Qt::MouseFocusReason);
  }
  QWidget::enterEvent(event);
}

bool CanvasView::event(QEvent* event) {
  const bool is_gesture = event->type() == QEvent::NativeGesture;
  if (is_gesture) {
    HandleGesture(event);
    return true;
  }
  return QWidget::event(event);
}

// Trackpad pinch zooms and two finger twist turns, toward the fingers.
void CanvasView::HandleGesture(QEvent* event) {
  const auto* gesture = static_cast<QNativeGestureEvent*>(event);
  const QPointF pos = gesture->position();
  switch (gesture->gestureType()) {
    case Qt::ZoomNativeGesture:
      view_.ZoomAt(pos, 1.0 + gesture->value());
      break;
    case Qt::RotateNativeGesture:
      view_.RotateAt(pos, gesture->value());
      break;
    default:
      return;
  }
  update();
}

void CanvasView::UpdateCursor() {
  if (drag_ == Drag::kRotate) {
    setCursor(Qt::SizeAllCursor);
  } else if (drag_ == Drag::kPan) {
    setCursor(Qt::ClosedHandCursor);
  } else if (space_held_) {
    setCursor(Qt::OpenHandCursor);
  } else {
    unsetCursor();
  }
}

}  // namespace snapper
