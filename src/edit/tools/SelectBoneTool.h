#pragma once

#include "edit/Tool.h"

namespace snapper {

// Picks bones on the canvas: click selects, Shift+click adds, Ctrl+click
// removes, a click on empty canvas selects none. Up and Down step to the
// selected bone's parent or child.
class SelectBoneTool : public Tool {
 public:
  explicit SelectBoneTool(ToolContext context);

  ToolInfo Info() const override;
  void Press(const ToolInput& input) override;
  bool KeyPress(int key, Qt::KeyboardModifiers modifiers) override;
};

}  // namespace snapper
