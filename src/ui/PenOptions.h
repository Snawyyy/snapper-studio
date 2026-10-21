#pragma once

#include <QWidget>

#include <functional>

#include "edit/tools/PenTool.h"

class QCheckBox;

namespace snapper {

class NumberField;

// The pen's settings in the Tool Options panel. Every change applies at
// once and is handed to `on_change` to be remembered.
class PenOptions : public QWidget {
  Q_OBJECT

 public:
  PenOptions(PenTool& pen, std::function<void(const PenSettings&)> on_change,
             QWidget* parent = nullptr);

 private:
  void Apply();

  PenTool& pen_;
  std::function<void(const PenSettings&)> on_change_;
  NumberField* smoothing_;
  NumberField* width_;
  QCheckBox* pressure_;
  QCheckBox* close_ends_;
  QCheckBox* fill_closed_;
};

}  // namespace snapper
