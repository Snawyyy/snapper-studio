#pragma once

#include "edit/Tool.h"

namespace snapper {

// Binds the active layer to a bone, Moho style: click a bone of the bone
// layer it sits in and the whole layer follows that bone. A click on empty
// canvas unbinds it.
class BindLayerTool : public Tool {
 public:
  explicit BindLayerTool(ToolContext context);

  ToolInfo Info() const override;
  void Press(const ToolInput& input) override;
  void DrawOverlay(QPainter& painter, const ViewTransform& view) const override;
};

}  // namespace snapper
