#include "edit/tools/LayerCheck.h"

#include "document/Document.h"
#include "edit/SelectionManager.h"
#include "edit/Tool.h"

namespace snapper {

bool CanDrawOnActiveLayer(const ToolContext& context) {
  const Layer* layer =
      context.document.Layers().Find(context.selection.ActiveLayer());
  const bool is_vector_layer =
      layer != nullptr && layer->type == LayerType::kVector;
  if (!is_vector_layer) {
    context.hint(QStringLiteral("Pick a vector layer to draw on"));
    return false;
  }
  if (!layer->visible) {
    context.hint(
        QStringLiteral("%1 is hidden: show it to draw on it").arg(layer->name));
    return false;
  }
  if (layer->locked) {
    context.hint(QStringLiteral("%1 is locked: unlock it to draw on it")
                     .arg(layer->name));
    return false;
  }
  return true;
}

}  // namespace snapper
