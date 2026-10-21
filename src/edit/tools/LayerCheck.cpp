#include "edit/tools/LayerCheck.h"

#include "document/Document.h"
#include "edit/SelectionManager.h"
#include "edit/Tool.h"

namespace snapper {

bool CanChangeActiveLayer(const ToolContext& context, const QString& action) {
  const Layer* layer =
      context.document.Layers().Find(context.selection.ActiveLayer());
  const bool is_vector_layer =
      layer != nullptr && layer->type == LayerType::kVector;
  if (!is_vector_layer) {
    context.hint(QStringLiteral("Pick a vector layer to %1").arg(action));
    return false;
  }
  if (!layer->visible) {
    context.hint(QStringLiteral("%1 is hidden: show it to %2 it")
                     .arg(layer->name, action));
    return false;
  }
  if (layer->locked) {
    context.hint(QStringLiteral("%1 is locked: unlock it to %2 it")
                     .arg(layer->name, action));
    return false;
  }
  return true;
}

}  // namespace snapper
