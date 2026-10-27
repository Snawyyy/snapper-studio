#include "edit/tools/AddBoneTool.h"

#include <QLineF>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <numbers>

#include "app/Theme.h"
#include "document/Document.h"
#include "edit/AnimationEditor.h"
#include "edit/BoneCommands.h"
#include "edit/MacroCommand.h"
#include "edit/SelectionCommands.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"
#include "edit/tools/LayerCheck.h"
#include "eval/FrameEvaluator.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

// Shorter than this on screen is a click, not a bone.
constexpr double kMinLengthPx = 4.0;
// Pressing this near the parent's tip starts the bone there.
constexpr double kSnapToTipPx = 12.0;
constexpr double kSnapAngle = std::numbers::pi / 12;

}  // namespace

AddBoneTool::AddBoneTool(ToolContext context) : Tool(std::move(context)) {}

ToolInfo AddBoneTool::Info() const {
  return {.id = QStringLiteral("add-bone"),
          .title = QStringLiteral("Add Bone"),
          .key = QKeySequence(Qt::Key_A),
          .hint = QStringLiteral(
              "Add Bone: drag out a bone. It joins the selected bone, so "
              "drags in a row grow a chain. Shift snaps the angle.")};
}

void AddBoneTool::Press(const ToolInput& input) {
  const Id bone_layer =
      BoneLayerToEdit(Context(), QStringLiteral("add bones to"));
  if (bone_layer == kNoId) {
    return;
  }
  if (!Context().animation.IsSetupFrame()) {
    Context().hint(QStringLiteral(
        "Bones are added on the setup frame: press Home to go there"));
    return;
  }
  const QSet<Id>& selected = Context().selection.Bones();
  parent_ = selected.size() == 1 ? *selected.cbegin() : kNoId;
  bone_layer_ = bone_layer;
  base_ = InBoneLayer(Context(), bone_layer, input.canvas);
  const PosedBone* parent = Context().frames.Current().FindBone(parent_);
  if (parent != nullptr) {
    const QTransform to_screen =
        Context().frames.Current().LayerMatrix(bone_layer) *
        input.view->ToScreenMatrix();
    const bool is_at_tip =
        QLineF(to_screen.map(parent->Tip()), input.screen).length() <=
        kSnapToTipPx;
    if (is_at_tip) {
      base_ = parent->Tip();
    }
  }
  tip_ = base_;
}

void AddBoneTool::Drag(const ToolInput& input) {
  if (bone_layer_ == kNoId) {
    return;
  }
  tip_ = InBoneLayer(Context(), bone_layer_, input.canvas);
  if (input.modifiers.testFlag(Qt::ShiftModifier)) {
    const QPointF span = tip_ - base_;
    const double length = std::hypot(span.x(), span.y());
    const double angle =
        std::round(std::atan2(span.y(), span.x()) / kSnapAngle) * kSnapAngle;
    tip_ = base_ + QPointF(length * std::cos(angle), length * std::sin(angle));
  }
}

void AddBoneTool::Release(const ToolInput& input) {
  if (bone_layer_ == kNoId) {
    return;
  }
  const Id bone_layer = bone_layer_;
  bone_layer_ = kNoId;
  const QPointF span = tip_ - base_;
  const double length = std::hypot(span.x(), span.y());
  if (length / input.canvas_per_screen < kMinLengthPx) {
    Context().hint(QStringLiteral("Drag to give the bone a length"));
    return;
  }
  const Pose& pose = Context().frames.Current();
  const PosedBone* parent = pose.FindBone(parent_);
  const double heading = std::atan2(span.y(), span.x());
  Bone bone{.id = Context().document.NewId(),
            .layer = bone_layer,
            .parent = parent == nullptr ? kNoId : parent_,
            .name = UnusedName(),
            .transform = {.position = base_, .angle = heading},
            .length = length};
  if (parent != nullptr) {
    bone.transform.position = parent->ChildPosition(base_);
    bone.transform.angle = heading - parent->Heading();
  }
  auto macro = std::make_unique<MacroCommand>(QStringLiteral("Add Bone"));
  macro->Add(std::make_unique<AddBoneCommand>(Context().document, bone));
  macro->Add(MakeSelectBones(Context().selection, {bone.id}));
  Context().undo.Push(std::move(macro));
  Context().hint(QStringLiteral("Added %1").arg(bone.name));
}

void AddBoneTool::Cancel() { bone_layer_ = kNoId; }

void AddBoneTool::DrawOverlay(QPainter& painter,
                              const ViewTransform& view) const {
  if (bone_layer_ == kNoId) {
    return;
  }
  const QTransform to_screen =
      Context().frames.Current().LayerMatrix(bone_layer_) *
      view.ToScreenMatrix();
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(
      QPen(Context().theme.accent, 2.0, Qt::SolidLine, Qt::RoundCap));
  painter.drawLine(to_screen.map(base_), to_screen.map(tip_));
  painter.restore();
}

QString AddBoneTool::UnusedName() const {
  const SkeletonManager& skeletons = Context().document.Skeletons();
  const int count = skeletons.Count();
  // The first free number. Of count + 1 numbers, at least one is free.
  for (int n = 1; n <= count + 1; ++n) {
    const QString name = QStringLiteral("Bone %1").arg(n);
    const bool is_taken = std::ranges::any_of(
        skeletons.All(), [&](const Bone& bone) { return bone.name == name; });
    if (!is_taken) {
      return name;
    }
  }
  return QStringLiteral("Bone");
}

}  // namespace snapper
