#pragma once

#include <QDoubleSpinBox>

namespace snapper {

// A number you can set three ways: drag sideways on it, scroll the wheel
// over it (Shift for bigger steps), or click and type.
class NumberField : public QDoubleSpinBox {
  Q_OBJECT

 public:
  explicit NumberField(QWidget* parent = nullptr);

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

 private:
  bool is_pressed_ = false;
  bool is_scrubbing_ = false;
  QPointF press_pos_;
  double press_value_ = 0.0;
};

}  // namespace snapper
