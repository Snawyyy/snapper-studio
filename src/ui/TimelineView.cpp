#include "ui/TimelineView.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

#include "document/Document.h"
#include "edit/Playhead.h"
#include "eval/Curves.h"

namespace snapper {

namespace {

constexpr int kLabelWidth = 64;
constexpr int kSetupWidth = 48;
constexpr int kRulerHeight = 22;
// Shorter than list rows, so every row fits a short panel. Still a big
// enough target for a pen.
constexpr int kRowHeight = 22;
constexpr int kKeyRadius = 5;
// Ruler numbers sit at least this far apart.
constexpr int kMinLabelGapPx = 36;

}  // namespace

TimelineView::TimelineView(const Document& document, Playhead& playhead,
                           KeyActions& keys, const Theme& theme,
                           QWidget* parent)
    : QWidget(parent),
      document_(document),
      playhead_(playhead),
      keys_(keys),
      theme_(theme) {
  setObjectName(QStringLiteral("timeline"));
  setMouseTracking(true);
  // Every row shows, however small the panel is made.
  setMinimumHeight(RowTop(kKeyRowCount) + 4);
  const auto repaint = [this] { update(); };
  connect(&playhead_, &Playhead::FrameChanged, this, [this](int frame) {
    KeepFrameInView(frame);
    update();
  });
  connect(&keys_, &KeyActions::KeysChanged, this, repaint);
  connect(&document_, &Document::CanvasChanged, this, repaint);
}

QSize TimelineView::sizeHint() const { return {600, RowTop(kKeyRowCount) + 4}; }

QSize TimelineView::minimumSizeHint() const {
  return {kLabelWidth + kSetupWidth + 100, RowTop(kKeyRowCount) + 4};
}

int TimelineView::RowTop(int row) const {
  return kRulerHeight + row * kRowHeight;
}

int TimelineView::RowAt(double y) const {
  if (y < kRulerHeight) {
    return -1;
  }
  return std::min(kKeyRowCount - 1,
                  static_cast<int>((y - kRulerHeight) / kRowHeight));
}

double TimelineView::FrameLeft(int frame) const {
  if (frame == 0) {
    return kLabelWidth;
  }
  return kLabelWidth + kSetupWidth + (frame - 1 - scroll_) * cell_px_;
}

double TimelineView::FrameCenter(int frame) const {
  return frame == 0 ? kLabelWidth + kSetupWidth / 2.0
                    : FrameLeft(frame) + cell_px_ / 2.0;
}

int TimelineView::FrameAt(double x) const {
  const double first = kLabelWidth + kSetupWidth;
  if (x < first) {
    return 0;
  }
  const int frame = 1 + scroll_ + static_cast<int>((x - first) / cell_px_);
  return std::clamp(frame, 0, playhead_.LastFrame());
}

std::optional<KeyRef> TimelineView::KeyAt(const QPointF& pos) const {
  const int row = RowAt(pos.y());
  const bool is_in_frames = row >= 0 && pos.x() >= kLabelWidth + kSetupWidth;
  if (!is_in_frames) {
    return std::nullopt;
  }
  const int frame = FrameAt(pos.x());
  const auto kind = static_cast<KeyRow>(row);
  const std::vector<int> frames = keys_.FramesIn(kind);
  if (std::ranges::binary_search(frames, frame)) {
    return KeyRef{.row = kind, .frame = frame};
  }
  return std::nullopt;
}

void TimelineView::KeepFrameInView(int frame) {
  if (frame == 0) {
    return;
  }
  const int shown =
      std::max(1, (width() - kLabelWidth - kSetupWidth) / cell_px_);
  if (frame - 1 < scroll_) {
    scroll_ = frame - 1;
  } else if (frame - 1 >= scroll_ + shown) {
    scroll_ = frame - shown;
  }
}

void TimelineView::paintEvent(QPaintEvent* /*event*/) {
  QPainter painter(this);
  painter.fillRect(rect(), theme_.window);
  DrawRows(painter);
  DrawRuler(painter);
  DrawKeys(painter);
  DrawPlayhead(painter);
}

void TimelineView::DrawRuler(QPainter& painter) const {
  painter.save();
  painter.fillRect(QRect(0, 0, width(), kRulerHeight), theme_.raised);
  QFont font = painter.font();
  font.setPixelSize(theme_.font_small);
  painter.setFont(font);
  const QRectF setup(kLabelWidth, 0, kSetupWidth, height());
  QColor setup_fill = theme_.setup_mode;
  setup_fill.setAlpha(60);
  painter.fillRect(setup, setup_fill);
  painter.setPen(theme_.setup_mode);
  painter.drawText(QRectF(kLabelWidth, 0, kSetupWidth, kRulerHeight),
                   Qt::AlignCenter, QStringLiteral("Setup"));

  const int every = std::max(1, (kMinLabelGapPx + cell_px_ - 1) / cell_px_);
  const int step =
      every <= 5 ? 5 : (every <= 10 ? 10 : 25 * ((every + 24) / 25));
  painter.setClipRect(QRect(kLabelWidth + kSetupWidth, 0, width(), height()));
  painter.setPen(theme_.text_secondary);
  for (int frame = 1 + scroll_; frame <= playhead_.LastFrame(); ++frame) {
    const double left = FrameLeft(frame);
    if (left > width()) {
      break;
    }
    const bool is_labeled = frame == 1 || frame % step == 0;
    if (is_labeled) {
      painter.drawText(QRectF(left - 20, 0, cell_px_ + 40, kRulerHeight),
                       Qt::AlignCenter, QString::number(frame));
    }
  }
  painter.restore();
}

void TimelineView::DrawRows(QPainter& painter) const {
  painter.save();
  QFont font = painter.font();
  font.setPixelSize(theme_.font_small);
  painter.setFont(font);
  const int last = playhead_.LastFrame();
  const double end = std::min<double>(width(), FrameLeft(last) + cell_px_);
  for (int row = 0; row < kKeyRowCount; ++row) {
    const int top = RowTop(row);
    painter.setPen(theme_.separator);
    painter.drawLine(0, top + kRowHeight, width(), top + kRowHeight);
    painter.setPen(row == 0 ? theme_.text : theme_.text_secondary);
    painter.drawText(QRect(8, top, kLabelWidth - 8, kRowHeight),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     KeyActions::RowName(static_cast<KeyRow>(row)));
  }
  // Every fifth frame is shaded a little, for counting at a glance.
  QColor shade = theme_.raised;
  shade.setAlpha(110);
  for (int frame = 1 + scroll_; frame <= last; ++frame) {
    const double left = FrameLeft(frame);
    if (left > width()) {
      break;
    }
    if (frame % 5 == 0) {
      painter.fillRect(QRectF(left, kRulerHeight, cell_px_, height()), shade);
    }
  }
  // Past the last frame there is nothing to animate.
  if (end < width()) {
    painter.fillRect(QRectF(end, kRulerHeight, width() - end, height()),
                     theme_.canvas_surround);
  }
  painter.setPen(theme_.separator);
  painter.drawLine(kLabelWidth, 0, kLabelWidth, height());
  painter.drawLine(kLabelWidth + kSetupWidth, 0, kLabelWidth + kSetupWidth,
                   height());
  painter.restore();
}

void TimelineView::DrawKeys(QPainter& painter) const {
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setClipRect(
      QRect(kLabelWidth + kSetupWidth, kRulerHeight, width(), height()));
  const double radius = std::min<double>(kKeyRadius, cell_px_ / 2.0 + 1);
  for (int row = 0; row < kKeyRowCount; ++row) {
    const auto kind = static_cast<KeyRow>(row);
    const double middle = RowTop(row) + kRowHeight / 2.0;
    for (const int frame : keys_.FramesIn(kind)) {
      const KeyRef key{.row = kind, .frame = frame};
      const bool is_selected = keys_.IsSelected(key);
      const bool is_dragged = is_selected && drag_ == Drag::kKeys;
      const int shown =
          is_dragged ? std::clamp(frame + key_shift_, 1, playhead_.LastFrame())
                     : frame;
      if (is_dragged && is_copying_) {
        // The original stays, faded, while its copy follows the pointer.
        painter.setOpacity(0.4);
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme_.text_secondary);
        painter.drawEllipse(QPointF(FrameCenter(frame), middle), radius,
                            radius);
        painter.setOpacity(1.0);
      }
      const QPointF center(FrameCenter(shown), middle);
      const std::optional<Curve> curve = keys_.CurveAt(kind, frame);
      // A step key is square, every other a diamond.
      QPainterPath mark;
      if (curve == Curve::kStep) {
        mark.addRect(QRectF(center - QPointF(radius - 1, radius - 1),
                            QSizeF(2 * radius - 2, 2 * radius - 2)));
      } else {
        mark.moveTo(center + QPointF(0, -radius));
        mark.lineTo(center + QPointF(radius, 0));
        mark.lineTo(center + QPointF(0, radius));
        mark.lineTo(center + QPointF(-radius, 0));
        mark.closeSubpath();
      }
      painter.setPen(QPen(is_selected ? Qt::white : theme_.window, 1.0));
      painter.setBrush(is_selected ? theme_.accent : theme_.text);
      painter.drawPath(mark);
    }
  }
  painter.restore();
}

void TimelineView::DrawPlayhead(QPainter& painter) const {
  const int frame = playhead_.Frame();
  const double x = FrameCenter(frame);
  const bool is_hidden = frame > 0 && x < kLabelWidth + kSetupWidth;
  if (is_hidden) {
    return;
  }
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  const QColor color = frame == 0 ? theme_.setup_mode : theme_.accent;
  painter.setPen(QPen(color, 1.5));
  painter.drawLine(QPointF(x, kRulerHeight), QPointF(x, height()));
  QFont font = painter.font();
  font.setPixelSize(theme_.font_small);
  font.setBold(true);
  painter.setFont(font);
  const QString label =
      frame == 0 ? QStringLiteral("Setup") : QString::number(frame);
  const double pill =
      std::max(28.0, painter.fontMetrics().horizontalAdvance(label) + 12.0);
  const QRectF box(x - pill / 2, 2, pill, kRulerHeight - 4);
  painter.setPen(Qt::NoPen);
  painter.setBrush(color);
  painter.drawRoundedRect(box, box.height() / 2, box.height() / 2);
  painter.setPen(Qt::white);
  painter.drawText(box, Qt::AlignCenter, label);
  painter.restore();
}

void TimelineView::mousePressEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton) {
    return;
  }
  const QPointF pos = event->position();
  if (pos.x() < kLabelWidth) {
    return;
  }
  const std::optional<KeyRef> key = KeyAt(pos);
  const Qt::KeyboardModifiers mods = event->modifiers();
  if (key.has_value()) {
    std::vector<KeyRef> selected = keys_.Selected();
    const bool is_selected = keys_.IsSelected(*key);
    if (mods.testFlag(Qt::ControlModifier)) {
      std::erase(selected, *key);
      if (!is_selected) {
        selected.push_back(*key);
      }
      keys_.Select(std::move(selected));
      return;
    }
    if (mods.testFlag(Qt::ShiftModifier)) {
      if (!is_selected) {
        selected.push_back(*key);
      }
      keys_.Select(std::move(selected));
    } else if (!is_selected) {
      keys_.Select({*key});
    }
    drag_ = Drag::kKeys;
    press_frame_ = key->frame;
    key_shift_ = 0;
    is_copying_ = mods.testFlag(Qt::AltModifier);
    playhead_.SetFrame(key->frame);
    return;
  }
  const bool is_adding =
      mods.testAnyFlags(Qt::ShiftModifier | Qt::ControlModifier);
  if (!is_adding) {
    keys_.Select({});
  }
  drag_ = Drag::kScrub;
  playhead_.SetFrame(FrameAt(pos.x()));
}

void TimelineView::mouseMoveEvent(QMouseEvent* event) {
  const QPointF pos = event->position();
  switch (drag_) {
    case Drag::kScrub:
      playhead_.SetFrame(FrameAt(pos.x()));
      break;
    case Drag::kKeys: {
      const int shift = std::max(1, FrameAt(pos.x())) - press_frame_;
      is_copying_ = event->modifiers().testFlag(Qt::AltModifier);
      if (shift != key_shift_) {
        key_shift_ = shift;
        update();
      }
      break;
    }
    case Drag::kNone: {
      const std::optional<KeyRef> key = KeyAt(pos);
      setCursor(key.has_value() ? Qt::SizeHorCursor : Qt::ArrowCursor);
      break;
    }
  }
}

void TimelineView::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton) {
    return;
  }
  const bool is_key_drop = drag_ == Drag::kKeys && key_shift_ != 0;
  drag_ = Drag::kNone;
  if (is_key_drop) {
    const int shift = key_shift_;
    key_shift_ = 0;
    keys_.MoveSelected(shift, is_copying_);
    playhead_.SetFrame(
        std::clamp(press_frame_ + shift, 1, playhead_.LastFrame()));
  }
  update();
}

void TimelineView::wheelEvent(QWheelEvent* event) {
  const int notches = event->angleDelta().y() / 120;
  const int sideways = event->angleDelta().x() / 120;
  if (event->modifiers().testFlag(Qt::ControlModifier)) {
    cell_px_ = std::clamp(cell_px_ + notches * 2, kMinCellPx, kMaxCellPx);
  } else {
    const int along = sideways != 0 ? -sideways : -notches;
    scroll_ = std::clamp(scroll_ + along * 5, 0,
                         std::max(0, playhead_.LastFrame() - 1));
  }
  update();
  event->accept();
}

// A right click on a key picks its curve.
void TimelineView::contextMenuEvent(QContextMenuEvent* event) {
  const std::optional<KeyRef> key = KeyAt(event->pos());
  if (!key.has_value()) {
    return;
  }
  if (!keys_.IsSelected(*key)) {
    keys_.Select({*key});
  }
  QMenu menu(this);
  const std::optional<Curve> current = keys_.SelectedCurve();
  for (int i = 0; i < kCurveCount; ++i) {
    const auto curve = static_cast<Curve>(i);
    QAction* action = menu.addAction(Curves::Name(curve));
    action->setCheckable(true);
    action->setChecked(current == curve);
    connect(action, &QAction::triggered, this,
            [this, curve] { keys_.SetCurve(curve); });
  }
  menu.exec(event->globalPos());
}

}  // namespace snapper
