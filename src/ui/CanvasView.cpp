#include "ui/CanvasView.h"

#include <QApplication>
#include <QLineEdit>
#include <QNativeGestureEvent>
#include <QPainter>
#include <QTabletEvent>
#include <QWheelEvent>

#include "app/CommandRegistry.h"
#include "document/Document.h"
#include "edit/Playhead.h"
#include "edit/SelectionManager.h"
#include "edit/ToolManager.h"
#include "eval/FrameEvaluator.h"
#include "render/OverlayRenderer.h"

namespace snapper {

namespace {

constexpr double kFitMargin = 48.0;
constexpr double kZoomStep = 1.25;
// How long the view must rest before slow art is drawn sharp again.
constexpr int kViewSettleMs = 120;

}  // namespace

CanvasView::CanvasView(Document& document, SelectionManager& selection,
                       FrameEvaluator& frames, const Playhead& playhead,
                       const Theme& theme, QWidget* parent)
    : QWidget(parent),
      document_(document),
      selection_(selection),
      frames_(frames),
      playhead_(playhead),
      theme_(theme) {
  setObjectName(QStringLiteral("canvas"));
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setAttribute(Qt::WA_AcceptTouchEvents);

  view_settle_.setSingleShot(true);
  view_settle_.setInterval(kViewSettleMs);
  connect(&view_settle_, &QTimer::timeout, this, [this] { update(); });

  const auto art_changed = [this] { ArtChanged(); };
  // Layers, shapes, channels and the frame all change the pose.
  connect(&frames_, &FrameEvaluator::PoseChanged, this, art_changed);
  connect(&playhead_, &Playhead::PlayingChanged, this, art_changed);
  connect(&document_.Styles(), &StyleManager::ListChanged, this, art_changed);
  connect(&document_.Styles(), &StyleManager::StyleChanged, this, art_changed);
  connect(&document_.Brushes(), &BrushManager::ListChanged, this, art_changed);
  connect(&document_.Brushes(), &BrushManager::BrushChanged, this, art_changed);
  connect(&document_, &Document::CanvasChanged, this, art_changed);
  const auto repaint = [this] { update(); };
  connect(&selection_, &SelectionManager::PointsChanged, this, repaint);
  connect(&selection_, &SelectionManager::ActiveLayerChanged, this, repaint);
  connect(&document_, &Document::Reset, this, [this] {
    art_.Invalidate();
    FitCanvas();
  });
}

void CanvasView::RegisterCommands(CommandRegistry& commands) {
  commands.Add({
      .id = QStringLiteral("view.fit-canvas"),
      .title = QStringLiteral("Fit Canvas"),
      .shortcut = QKeySequence(Qt::SHIFT | Qt::Key_F),
      .run = [this] { FitCanvas(); },
  });
  commands.Add({
      .id = QStringLiteral("view.fit-selection"),
      .title = QStringLiteral("Fit Selection"),
      .shortcut = QKeySequence(Qt::Key_F),
      .run = [this] { FitSelection(); },
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
  commands.Add({
      .id = QStringLiteral("view.onion-skin"),
      .title = QStringLiteral("Onion Skin"),
      .shortcut = QKeySequence(Qt::Key_O),
      .run =
          [this] {
            OnionSettings onion = onion_;
            onion.on = !onion.on;
            SetOnion(onion);
          },
  });
}

void CanvasView::HighlightStyle(Id style) {
  if (style != highlighted_style_) {
    highlighted_style_ = style;
    update();
  }
}

void CanvasView::SetOnion(const OnionSettings& onion) {
  onion_ = onion;
  ArtChanged();
  Q_EMIT OnionChanged();
}

void CanvasView::SetTools(ToolManager* tools) {
  tools_ = tools;
  connect(tools_, &ToolManager::OverlayChanged, this, [this] { update(); });
}

void CanvasView::FitCanvas() {
  const QSizeF size = document_.Canvas().size.toSizeF();
  view_.Fit(QRectF(QPointF(0, 0), size), kFitMargin);
  is_fit_view_ = true;
  update();
}

// With nothing selected, fits the canvas instead. A single point is shown
// at the zoom it already has.
void CanvasView::FitSelection() {
  const std::optional<QRectF> bounds = selection_.Bounds();
  if (!bounds.has_value()) {
    FitCanvas();
    return;
  }
  const bool is_one_spot = bounds->width() < 1 && bounds->height() < 1;
  if (is_one_spot) {
    view_.PanBy(view_.ToScreen(view_.Center()) -
                view_.ToScreen(bounds->center()));
  } else {
    view_.Fit(*bounds, kFitMargin);
  }
  ViewMoved();
}

void CanvasView::ZoomAround(double factor) {
  view_.ZoomAt(rect().center(), factor);
  ViewMoved();
}

void CanvasView::ActualSize() {
  view_.ResetZoom();
  ViewMoved();
}

void CanvasView::paintEvent(QPaintEvent* /*event*/) {
  QPainter painter(this);
  painter.fillRect(rect(), theme_.canvas_surround);
  painter.setRenderHint(QPainter::Antialiasing);
  // Onion skin stays out of the way while playing.
  std::vector<OnionFrame> onion;
  if (!playhead_.IsPlaying()) {
    for (const int frame :
         OnionSkin::Frames(onion_, playhead_.Frame(), playhead_.LastFrame())) {
      onion.push_back(
          {.pose = &frames_.At(frame), .offset = frame - playhead_.Frame()});
    }
  }
  const Pose& pose = frames_.Current();
  art_.Draw(painter, document_, pose, onion, view_.ToScreenMatrix(), size(),
            devicePixelRatioF(), view_settle_.isActive());
  OverlayRenderer::DrawSurround(painter, document_, view_, rect(),
                                theme_.canvas_surround);
  OverlayRenderer::DrawStyleUsers(painter, document_, pose, view_,
                                  highlighted_style_, theme_.accent);
  if (tools_ != nullptr) {
    painter.setClipping(false);
    tools_->DrawOverlay(painter, view_);
  }
}

void CanvasView::resizeEvent(QResizeEvent* event) {
  view_.SetViewSize(size().toSizeF());
  if (is_fit_view_) {
    FitCanvas();
  }
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
  navigator_.Wheel(event->position(), event->angleDelta().y());
  ViewMoved();
  event->accept();
}

void CanvasView::ViewMoved() {
  is_fit_view_ = false;
  view_settle_.start();
  update();
}

void CanvasView::ArtChanged() {
  art_.Invalidate();
  update();
}

bool CanvasView::StartViewDrag(Qt::MouseButton button,
                               Qt::KeyboardModifiers modifiers,
                               const QPointF& pos) {
  if (!navigator_.StartDrag(button, modifiers, pos)) {
    return false;
  }
  ViewMoved();
  UpdateCursor();
  return true;
}

ToolInput CanvasView::MakeInput(const QPointF& screen, float pressure,
                                Qt::KeyboardModifiers modifiers) const {
  return {.canvas = view_.ToCanvas(screen),
          .screen = screen,
          .pressure = pressure,
          .modifiers = modifiers,
          .canvas_per_screen = 1.0 / view_.Zoom(),
          .view = &view_};
}

void CanvasView::mousePressEvent(QMouseEvent* event) {
  const QPointF pos = event->position();
  if (StartViewDrag(event->button(), event->modifiers(), pos)) {
    return;
  }
  const bool is_tool_press =
      event->button() == Qt::LeftButton && tools_ != nullptr;
  if (is_tool_press) {
    is_tool_pressed_ = true;
    tools_->Press(MakeInput(pos, 1.0F, event->modifiers()));
  }
}

void CanvasView::mouseMoveEvent(QMouseEvent* event) {
  const QPointF pos = event->position();
  if (navigator_.IsDragging()) {
    navigator_.DragTo(pos, rect().toRectF());
    ViewMoved();
    return;
  }
  if (tools_ == nullptr) {
    return;
  }
  const ToolInput input = MakeInput(pos, 1.0F, event->modifiers());
  if (is_tool_pressed_) {
    tools_->Drag(input);
  } else {
    tools_->Hover(input);
  }
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event) {
  if (navigator_.IsDragging()) {
    navigator_.EndDrag();
    UpdateCursor();
    return;
  }
  const bool is_tool_release =
      is_tool_pressed_ && event->button() == Qt::LeftButton;
  if (is_tool_release) {
    is_tool_pressed_ = false;
    tools_->Release(MakeInput(event->position(), 1.0F, event->modifiers()));
  }
}

void CanvasView::mouseDoubleClickEvent(QMouseEvent* event) {
  const bool is_tool_click = event->button() == Qt::LeftButton &&
                             tools_ != nullptr && !navigator_.IsSpaceHeld();
  if (is_tool_click) {
    tools_->DoubleClick(MakeInput(event->position(), 1.0F, event->modifiers()));
  }
}

// A pen gives pressure. Accepting its events stops Qt from also sending
// them again as mouse events.
void CanvasView::tabletEvent(QTabletEvent* event) {
  const QPointF pos = event->position();
  const auto pressure = static_cast<float>(event->pressure());
  switch (event->type()) {
    case QEvent::TabletPress: {
      if (StartViewDrag(event->button(), event->modifiers(), pos)) {
        break;
      }
      const bool is_tool_press =
          tools_ != nullptr && event->button() == Qt::LeftButton;
      if (is_tool_press) {
        is_tool_pressed_ = true;
        tools_->Press(MakeInput(pos, pressure, event->modifiers()));
      }
      break;
    }
    case QEvent::TabletMove: {
      const bool is_not_for_tools =
          navigator_.IsDragging() || tools_ == nullptr;
      if (is_not_for_tools) {
        // Pan and turn follow the same path as the mouse.
        event->ignore();
        return;
      }
      if (is_tool_pressed_) {
        tools_->Drag(MakeInput(pos, pressure, event->modifiers()));
      } else {
        tools_->Hover(MakeInput(pos, pressure, event->modifiers()));
      }
      break;
    }
    case QEvent::TabletRelease:
      if (navigator_.IsDragging()) {
        event->ignore();
        return;
      }
      if (is_tool_pressed_) {
        is_tool_pressed_ = false;
        tools_->Release(MakeInput(pos, pressure, event->modifiers()));
      }
      break;
    default:
      event->ignore();
      return;
  }
  event->accept();
}

void CanvasView::leaveEvent(QEvent* event) {
  if (tools_ != nullptr) {
    tools_->Leave();
  }
  QWidget::leaveEvent(event);
}

void CanvasView::keyPressEvent(QKeyEvent* event) {
  const bool is_cancel = event->key() == Qt::Key_Escape && tools_ != nullptr;
  if (is_cancel) {
    is_tool_pressed_ = false;
    tools_->Cancel();
    return;
  }
  const bool is_used_by_tool =
      tools_ != nullptr && tools_->KeyPressed(event->key(), event->modifiers());
  if (is_used_by_tool) {
    return;
  }
  const bool is_space = event->key() == Qt::Key_Space;
  if (!is_space) {
    QWidget::keyPressEvent(event);
    return;
  }
  if (!event->isAutoRepeat()) {
    navigator_.SetSpaceHeld(true);
    UpdateCursor();
  }
  event->accept();
}

void CanvasView::keyReleaseEvent(QKeyEvent* event) {
  const bool is_tool_key_up = !event->isAutoRepeat() && tools_ != nullptr;
  if (is_tool_key_up) {
    tools_->KeyReleased(event->key());
  }
  const bool is_space = event->key() == Qt::Key_Space;
  if (!is_space) {
    QWidget::keyReleaseEvent(event);
    return;
  }
  if (!event->isAutoRepeat()) {
    navigator_.SetSpaceHeld(false);
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

// Trackpad pinch zooms and two finger twist turns, toward the fingers.
bool CanvasView::event(QEvent* event) {
  const bool is_gesture = event->type() == QEvent::NativeGesture;
  if (!is_gesture) {
    return QWidget::event(event);
  }
  const auto* gesture = static_cast<QNativeGestureEvent*>(event);
  const bool is_used = navigator_.Gesture(
      gesture->gestureType(), gesture->value(), gesture->position());
  if (is_used) {
    ViewMoved();
  }
  return true;
}

void CanvasView::UpdateCursor() {
  const std::optional<Qt::CursorShape> cursor = navigator_.Cursor();
  if (cursor.has_value()) {
    setCursor(*cursor);
  } else {
    unsetCursor();
  }
}

}  // namespace snapper
