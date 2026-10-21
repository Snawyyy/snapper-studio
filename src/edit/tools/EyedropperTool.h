#pragma once

#include <QColor>

#include <cstdint>
#include <optional>

#include "document/ShapeManager.h"
#include "edit/Tool.h"

namespace snapper {

// Picks a look from the artwork into the selected shapes, or into the look
// for new shapes. In color mode a click on a line takes its line color,
// inside a fill its fill color, and on a picture the pixel's color as the
// line color. In style mode it takes a shape's whole look and styles. Alt
// picks the other way for one click.
class EyedropperTool : public Tool {
 public:
  enum class Mode : std::uint8_t { kColor, kWholeStyle };

  // What lies under the pointer.
  struct Pick {
    enum class Kind : std::uint8_t { kLine, kFill, kPicture };
    Kind kind = Kind::kLine;
    QColor color;
    // The shape's own look and styles, unless it's a picture.
    std::optional<ShapeLookState> shape;
  };

  explicit EyedropperTool(ToolContext context);

  ToolInfo Info() const override;
  void Press(const ToolInput& input) override;
  void Hover(const ToolInput& input) override;
  void Leave() override { hover_.reset(); }
  void Cancel() override { hover_.reset(); }
  void DrawOverlay(QPainter& painter, const ViewTransform& view) const override;

  Mode CurrentMode() const { return mode_; }
  void SetMode(Mode mode) { mode_ = mode; }

  // The topmost thing on a visible layer under the pointer.
  std::optional<Pick> PickAt(const ToolInput& input) const;

 private:
  void Apply(const Pick& pick, Mode mode);

  Mode mode_ = Mode::kColor;
  std::optional<Pick> hover_;
  QPointF hover_screen_;
};

}  // namespace snapper
