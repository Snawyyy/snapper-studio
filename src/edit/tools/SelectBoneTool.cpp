#include "edit/tools/SelectBoneTool.h"

#include "document/Document.h"
#include "edit/HitTest.h"
#include "edit/SelectionCommands.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"
#include "eval/FrameEvaluator.h"

namespace snapper {

SelectBoneTool::SelectBoneTool(ToolContext context)
    : Tool(std::move(context)) {}

ToolInfo SelectBoneTool::Info() const {
  return {.id = QStringLiteral("select-bone"),
          .title = QStringLiteral("Select Bone"),
          .key = QKeySequence(Qt::Key_B),
          .hint = QStringLiteral(
              "Select Bone: click a bone, Shift+click adds, Ctrl+click "
              "removes. Up and Down step along the chain.")};
}

void SelectBoneTool::Press(const ToolInput& input) {
  SelectionManager& selection = Context().selection;
  const Id bone_layer = selection.BoneLayer();
  if (bone_layer == kNoId) {
    Context().hint(QStringLiteral("Pick a bone layer to select its bones"));
    return;
  }
  const BoneHit hit = HitTest::BoneAt(Context().frames.Current(), bone_layer,
                                      *input.view, input.screen);
  QSet<Id> after;
  if (hit.bone != kNoId) {
    after = {hit.bone};
    if (input.modifiers.testFlag(Qt::ShiftModifier)) {
      after = QSet<Id>(selection.Bones()).unite(after);
    } else if (input.modifiers.testFlag(Qt::ControlModifier)) {
      after = QSet<Id>(selection.Bones()).subtract(after);
    }
  }
  Context().undo.Push(MakeSelectBones(selection, after));
  const Bone* bone = Context().document.Skeletons().Find(hit.bone);
  if (bone != nullptr) {
    Context().hint(bone->name);
  }
}

bool SelectBoneTool::KeyPress(int key, Qt::KeyboardModifiers /*modifiers*/) {
  const bool is_step = key == Qt::Key_Up || key == Qt::Key_Down;
  SelectionManager& selection = Context().selection;
  if (!is_step || selection.Bones().size() != 1) {
    return false;
  }
  const SkeletonManager& skeletons = Context().document.Skeletons();
  const Bone* bone = skeletons.Find(*selection.Bones().cbegin());
  if (bone == nullptr) {
    return false;
  }
  Id next = kNoId;
  if (key == Qt::Key_Up) {
    next = bone->parent;
  } else {
    const std::vector<Id> children = skeletons.ChildrenOf(bone->id);
    next = children.empty() ? kNoId : children.front();
  }
  if (next == kNoId) {
    return true;
  }
  Context().undo.Push(MakeSelectBones(selection, {next}));
  Context().hint(skeletons.Find(next)->name);
  return true;
}

}  // namespace snapper
