#include "edit/LayerActions.h"

#include <algorithm>

#include "app/CommandRegistry.h"
#include "document/Document.h"
#include "edit/LayerCommands.h"
#include "edit/SelectionManager.h"
#include "edit/UndoManager.h"

namespace snapper {

LayerActions::LayerActions(Document& document, SelectionManager& selection,
                           UndoManager& undo,
                           std::function<void(const QString&)> hint)
    : document_(document),
      selection_(selection),
      undo_(undo),
      hint_(std::move(hint)) {}

void LayerActions::AddLayer() {
  const Layer layer{.id = document_.NewId(), .name = UnusedName()};
  const int above_active =
      document_.Layers().IndexOf(selection_.ActiveLayer()) + 1;
  undo_.Push(std::make_unique<AddLayerCommand>(document_, layer, above_active));
  selection_.SetActiveLayer(layer.id);
}

bool LayerActions::DeleteLayer(Id layer, QString* why_not) {
  const bool is_last = document_.Layers().Count() <= 1;
  if (is_last) {
    *why_not = QStringLiteral("A project keeps at least one layer");
    return false;
  }
  if (document_.Layers().Find(layer) == nullptr) {
    *why_not = QStringLiteral("Pick a layer first");
    return false;
  }
  undo_.Push(std::make_unique<RemoveLayerCommand>(document_, layer));
  return true;
}

void LayerActions::RenameLayer(Id layer, const QString& name) {
  const Layer* found = document_.Layers().Find(layer);
  const QString trimmed = name.trimmed();
  const bool is_change =
      found != nullptr && !trimmed.isEmpty() && found->name != trimmed;
  if (is_change) {
    undo_.Push(std::make_unique<RenameLayerCommand>(document_, layer, trimmed));
  }
}

void LayerActions::SetVisible(Id layer, bool visible) {
  const Layer* found = document_.Layers().Find(layer);
  const bool is_change = found != nullptr && found->visible != visible;
  if (is_change) {
    undo_.Push(
        std::make_unique<SetLayerVisibleCommand>(document_, layer, visible));
  }
}

void LayerActions::SetLocked(Id layer, bool locked) {
  const Layer* found = document_.Layers().Find(layer);
  const bool is_change = found != nullptr && found->locked != locked;
  if (is_change) {
    undo_.Push(
        std::make_unique<SetLayerLockedCommand>(document_, layer, locked));
  }
}

void LayerActions::MoveLayer(Id layer, int steps) {
  const int from = document_.Layers().IndexOf(layer);
  const int to = from + steps;
  const bool is_possible =
      from >= 0 && to >= 0 && to < document_.Layers().Count();
  if (is_possible) {
    undo_.Push(std::make_unique<MoveLayerCommand>(document_, layer, to));
  }
}

void LayerActions::RegisterCommands(CommandRegistry& commands) {
  commands.Add({
      .id = QStringLiteral("layer.add"),
      .title = QStringLiteral("New Layer"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N),
      .run = [this] { AddLayer(); },
  });
  commands.Add({
      .id = QStringLiteral("layer.delete"),
      .title = QStringLiteral("Delete Layer"),
      .shortcut = {},
      .run =
          [this] {
            QString why_not;
            if (!DeleteLayer(selection_.ActiveLayer(), &why_not)) {
              hint_(why_not);
            }
          },
  });
  commands.Add({
      .id = QStringLiteral("layer.lock"),
      .title = QStringLiteral("Lock or Unlock Layer"),
      .shortcut = QKeySequence(Qt::Key_L),
      .run =
          [this] {
            const Layer* layer =
                document_.Layers().Find(selection_.ActiveLayer());
            if (layer != nullptr) {
              SetLocked(layer->id, !layer->locked);
            }
          },
  });
  commands.Add({
      .id = QStringLiteral("layer.move-up"),
      .title = QStringLiteral("Move Layer Up"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::Key_BracketRight),
      .run = [this] { MoveLayer(selection_.ActiveLayer(), 1); },
  });
  commands.Add({
      .id = QStringLiteral("layer.move-down"),
      .title = QStringLiteral("Move Layer Down"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::Key_BracketLeft),
      .run = [this] { MoveLayer(selection_.ActiveLayer(), -1); },
  });
}

QString LayerActions::UnusedName() const {
  // Of count + 1 numbers in a row, at least one is free.
  const std::vector<Layer>& layers = document_.Layers().All();
  const int count = static_cast<int>(layers.size());
  for (int n = count + 1; n <= 2 * count + 1; ++n) {
    const QString name = QStringLiteral("Layer %1").arg(n);
    const bool is_taken = std::ranges::any_of(
        layers, [&](const Layer& layer) { return layer.name == name; });
    if (!is_taken) {
      return name;
    }
  }
  return QStringLiteral("Layer");
}

}  // namespace snapper
