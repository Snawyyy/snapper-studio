#pragma once

#include <QWidget>

#include <functional>

#include "edit/tools/ShapeTool.h"

class QCheckBox;

namespace snapper {

class NumberField;

// A rectangle or ellipse tool's settings in the Tool Options panel.
class ShapeOptions : public QWidget {
  Q_OBJECT

 public:
  ShapeOptions(ShapeTool& tool,
               std::function<void(const ShapeToolSettings&)> on_change,
               QWidget* parent = nullptr);

 private:
  void Apply();

  ShapeTool& tool_;
  std::function<void(const ShapeToolSettings&)> on_change_;
  QCheckBox* fill_;
  QCheckBox* line_;
  NumberField* width_;
};

}  // namespace snapper
