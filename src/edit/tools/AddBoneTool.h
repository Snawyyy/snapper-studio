#pragma once

#include <QPointF>

#include "document/Ids.h"
#include "edit/Tool.h"

namespace snapper {

// Drags out new bones on the setup frame, Moho style. A new bone joins the
// selected bone as its child, starting at its tip when pressed near it,
// and becomes the selected one, so a chain grows drag by drag. Shift
// snaps its angle to 15 degrees.
class AddBoneTool : public Tool {
 public:
  explicit AddBoneTool(ToolContext context);

  ToolInfo Info() const override;
  void Press(const ToolInput& input) override;
  void Drag(const ToolInput& input) override;
  void Release(const ToolInput& input) override;
  void Cancel() override;
  void DrawOverlay(QPainter& painter, const ViewTransform& view) const override;
  bool IsBusy() const override { return bone_layer_ != kNoId; }

 private:
  QString UnusedName() const;

  // Set while a bone is being dragged out.
  Id bone_layer_ = kNoId;
  Id parent_ = kNoId;
  // In bone layer space.
  QPointF base_;
  QPointF tip_;
};

}  // namespace snapper
