#pragma once

#include <QColor>
#include <QFrame>

#include <functional>

class QLineEdit;

namespace snapper {

class ColorPlane;
class ColorStrip;

// A color picker that floats over the window: a saturation and brightness
// square, hue and opacity strips, and a hex field. Every change shows at
// once. A click outside keeps the color, Escape puts the first one back.
class ColorPopover : public QFrame {
  Q_OBJECT

 public:
  // Lives inside `window` and never reaches past its edge.
  explicit ColorPopover(QWidget* window);

  // Each change of the color, with the drag tag all changes of one open
  // share, so they make one undo step.
  using OnChange = std::function<void(const QColor& color, int drag_tag)>;

  // Opens below `anchor`, starting from `color`, and reports changes to
  // `on_change` until it closes.
  void Open(QWidget* anchor, const QColor& color, OnChange on_change);
  void Close();
  bool IsOpenFor(const QWidget* anchor) const;

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  // Sets the shown color from hue, saturation, value and alpha, and
  // reports it.
  void SetHsva(float hue, float saturation, float value, float alpha);
  void ShowColor();
  void Report(const QColor& color);
  void ReadHex();

  ColorPlane* plane_;
  ColorStrip* hue_;
  ColorStrip* alpha_;
  QLineEdit* hex_;
  OnChange on_change_;
  const QWidget* anchor_ = nullptr;
  QColor first_;
  float hue_value_ = 0.0F;
  float saturation_ = 0.0F;
  float value_ = 0.0F;
  float alpha_value_ = 1.0F;
  int drag_tag_ = 0;
};

}  // namespace snapper
