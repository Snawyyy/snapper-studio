#include "edit/tools/BindLayerTool.h"

#include <QPainter>

#include "app/Theme.h"
#include "document/Document.h"
#include "edit/BoneCommands.h"
#include "edit/HitTest.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"
#include "eval/FrameEvaluator.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

constexpr double kBoundWidthPx = 5.0;

}  // namespace

BindLayerTool::BindLayerTool(ToolContext context) : Tool(std::move(context)) {}

ToolInfo BindLayerTool::Info() const {
  return {.id = QStringLiteral("bind-layer"),
          .title = QStringLiteral("Bind Layer"),
          .key = QKeySequence(Qt::SHIFT | Qt::Key_B),
          .hint = QStringLiteral(
              "Bind Layer: click a bone and the active layer follows it. "
              "Click empty canvas to unbind it.")};
}

void BindLayerTool::Press(const ToolInput& input) {
  const Document& document = Context().document;
  const Layer* layer =
      document.Layers().Find(Context().selection.ActiveLayer());
  const Id bone_layer =
      layer == nullptr ? kNoId : document.Layers().BoneLayerOf(layer->parent);
  if (bone_layer == kNoId) {
    Context().hint(QStringLiteral(
        "Pick a layer inside a bone layer to bind it to a bone"));
    return;
  }
  const BoneHit hit = HitTest::BoneAt(Context().frames.Current(), bone_layer,
                                      *input.view, input.screen);
  const Id bound = document.Bindings().BoneOfLayer(layer->id);
  if (hit.bone == bound) {
    return;
  }
  Context().undo.Push(std::make_unique<BindLayerCommand>(Context().document,
                                                         layer->id, hit.bone));
  const Bone* bone = document.Skeletons().Find(hit.bone);
  Context().hint(
      bone == nullptr
          ? QStringLiteral("%1 follows no bone").arg(layer->name)
          : QStringLiteral("%1 follows %2").arg(layer->name, bone->name));
}

// The bone the active layer follows, drawn thick.
void BindLayerTool::DrawOverlay(QPainter& painter,
                                const ViewTransform& view) const {
  const Document& document = Context().document;
  const Id layer = Context().selection.ActiveLayer();
  const Pose& pose = Context().frames.Current();
  const PosedBone* bone = pose.FindBone(document.Bindings().BoneOfLayer(layer));
  if (bone == nullptr) {
    return;
  }
  const QTransform to_screen =
      pose.LayerMatrix(bone->layer) * view.ToScreenMatrix();
  QColor color = Context().theme.accent;
  color.setAlphaF(0.6F);
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(color, kBoundWidthPx, Qt::SolidLine, Qt::RoundCap));
  painter.drawLine(to_screen.map(bone->Base()), to_screen.map(bone->Tip()));
  painter.restore();
}

}  // namespace snapper
