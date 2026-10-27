#pragma once

#include <QWidget>

#include <optional>

#include "app/Theme.h"
#include "edit/KeyActions.h"

namespace snapper {

class Document;
class Playhead;

// The frames and keys of the active layer, Moho style: a row with every
// key, then one per kind of channel. Frame 0 is the Setup cell, set apart
// in the setup orange. Click or drag on the frames to scrub, click keys to
// select them, drag them to retime and Alt+drag to copy. The wheel scrolls
// along, Ctrl+wheel zooms.
class TimelineView : public QWidget {
  Q_OBJECT

 public:
  static constexpr int kMinCellPx = 6;
  static constexpr int kMaxCellPx = 48;

  TimelineView(const Document& document, Playhead& playhead, KeyActions& keys,
               const Theme& theme, QWidget* parent = nullptr);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;

 private:
  enum class Drag : std::uint8_t { kNone, kScrub, kKeys };

  int RowTop(int row) const;
  // -1 above the rows.
  int RowAt(double y) const;
  // The left edge of a frame's cell.
  double FrameLeft(int frame) const;
  double FrameCenter(int frame) const;
  // The frame under an x position, clamped to the frames there are.
  int FrameAt(double x) const;
  std::optional<KeyRef> KeyAt(const QPointF& pos) const;
  void KeepFrameInView(int frame);

  void DrawRuler(QPainter& painter) const;
  void DrawRows(QPainter& painter) const;
  void DrawKeys(QPainter& painter) const;
  void DrawPlayhead(QPainter& painter) const;

  const Document& document_;
  Playhead& playhead_;
  KeyActions& keys_;
  const Theme& theme_;
  int cell_px_ = 14;
  // Frames scrolled off the left.
  int scroll_ = 0;
  Drag drag_ = Drag::kNone;
  int press_frame_ = 0;
  // How far dragged keys have moved, in frames.
  int key_shift_ = 0;
  bool is_copying_ = false;
};

}  // namespace snapper
