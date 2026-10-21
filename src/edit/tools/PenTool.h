#pragma once

#include <QColor>

#include <memory>

#include "document/ShapeManager.h"
#include "edit/Tool.h"
#include "edit/tools/PenStroke.h"

namespace snapper {

struct PenSettings {
  // 0 follows the pen exactly, 1 is the smoothest.
  double smoothing = 0.5;
  double line_width = 3.0;
  bool pressure_width = true;
  // A stroke ending near where it began becomes a closed shape.
  bool close_ends = true;
  bool fill_closed = false;
};

// Draws freehand lines on the active layer. The line is smoothed as it is
// drawn, its width follows pen pressure, and when the pen lifts it becomes
// a shape with as few points as keep its form.
class PenTool : public Tool {
 public:
  static constexpr double kMinLineWidth = 0.5;
  static constexpr double kMaxLineWidth = 200.0;

  explicit PenTool(ToolContext context);
  ~PenTool() override;

  ToolInfo Info() const override;
  void Press(const ToolInput& input) override;
  void Drag(const ToolInput& input) override;
  void Release(const ToolInput& input) override;
  void Hover(const ToolInput& input) override;
  void Leave() override { is_hovering_ = false; }
  void Cancel() override;
  void DrawOverlay(QPainter& painter, const ViewTransform& view) const override;
  bool IsBusy() const override { return stroke_ != nullptr; }

  const PenSettings& Settings() const { return settings_; }
  void SetSettings(const PenSettings& settings);

 private:
  // The shape the stroke would make if the pen lifted now.
  Shape PreviewShape() const;
  float WidthFor(float pressure) const;

  PenSettings settings_;
  std::unique_ptr<PenStroke> stroke_;
  QPointF pen_pos_;
  float pen_pressure_ = 1.0F;
  bool is_hovering_ = false;
};

}  // namespace snapper
