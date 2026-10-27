#pragma once

#include <QPointF>

#include <vector>

#include "document/Ids.h"
#include "edit/Tool.h"

namespace snapper {

// Poses bones by pulling them, Moho style: the spot you grab follows the
// pointer and the chain above it bends to reach, with IK, up to a pinned
// bone or the root. On the setup frame this only tests the rig and snaps
// back on release. On other frames it sets keys.
class ManipulateBonesTool : public Tool {
 public:
  explicit ManipulateBonesTool(ToolContext context);

  ToolInfo Info() const override;
  void Press(const ToolInput& input) override;
  void Drag(const ToolInput& input) override;
  void Release(const ToolInput& input) override;
  void Cancel() override;
  bool IsBusy() const override { return bone_ != kNoId; }

 private:
  // Ends a drag: drops a setup frame test, or keeps the keys.
  void Finish(bool is_cancel);

  Id bone_layer_ = kNoId;
  // The bone grabbed, kNoId when not dragging.
  Id bone_ = kNoId;
  // How far along the bone it was grabbed, in its own units.
  double grab_ = 0.0;
  // The grabbed bone and the bones above it that bend, root first.
  std::vector<Id> chain_;
  int drag_tag_ = 0;
  bool has_moved_ = false;
};

}  // namespace snapper
