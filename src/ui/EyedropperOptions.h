#pragma once

#include <QWidget>

#include "edit/tools/EyedropperTool.h"

class QPushButton;

namespace snapper {

// The eyedropper's settings in the Tool Options panel: whether a click
// picks a color or a shape's whole style.
class EyedropperOptions : public QWidget {
  Q_OBJECT

 public:
  explicit EyedropperOptions(EyedropperTool& tool, QWidget* parent = nullptr);

  // Shows the tool's mode after it changed elsewhere.
  void Refresh();

 private:
  EyedropperTool& tool_;
  QPushButton* color_;
  QPushButton* style_;
};

}  // namespace snapper
