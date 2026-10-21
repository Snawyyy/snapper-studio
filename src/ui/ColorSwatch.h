#pragma once

#include <QAbstractButton>
#include <QColor>

namespace snapper {

class ColorPopover;

// A small rounded patch of color. Clicking it opens the color picker, and
// every change in the picker shows here and goes to ColorChanged.
class ColorSwatch : public QAbstractButton {
  Q_OBJECT

 public:
  ColorSwatch(ColorPopover& popover, QWidget* parent = nullptr);

  QColor Color() const { return color_; }
  // Shows the color without reporting it.
  void SetColor(const QColor& color);
  QSize sizeHint() const override;

 Q_SIGNALS:
  void ColorChanged(const QColor& color, int drag_tag);

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  ColorPopover& popover_;
  QColor color_ = Qt::black;
};

}  // namespace snapper
