#include "edit/LayerActions.h"

#include <QFile>
#include <QFileInfo>

#include <algorithm>

#include "app/CommandRegistry.h"
#include "document/Document.h"
#include "edit/LayerCommands.h"
#include "edit/MacroCommand.h"
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
  const LayerManager& layers = document_.Layers();
  const Layer* active = layers.Find(selection_.ActiveLayer());
  const Layer layer{.id = document_.NewId(),
                    .parent = active == nullptr ? kNoId : active->parent,
                    .name = UnusedName(QStringLiteral("Layer"))};
  const int above_active =
      active == nullptr ? layers.Count() : layers.EndOf(active->id);
  undo_.Push(std::make_unique<AddLayerCommand>(document_, layer, above_active));
  selection_.SetActiveLayer(layer.id);
}

void LayerActions::AddBoneLayer() {
  const LayerManager& layers = document_.Layers();
  const Layer* active = layers.Find(selection_.ActiveLayer());
  if (active == nullptr) {
    hint_(QStringLiteral("Pick a layer to put in the bone layer"));
    return;
  }
  const Id wrapped = active->id;
  const Layer bones{.id = document_.NewId(),
                    .parent = active->parent,
                    .type = LayerType::kBone,
                    .name = UnusedName(QStringLiteral("Bones"))};
  const int at = layers.IndexOf(wrapped);
  auto macro = std::make_unique<MacroCommand>(QStringLiteral("New Bone Layer"));
  macro->Add(std::make_unique<AddLayerCommand>(document_, bones, at));
  // Once the bone layer is in, the wrapped layer sits just above it.
  macro->Add(
      std::make_unique<MoveLayerCommand>(document_, wrapped, bones.id, at + 1));
  undo_.Push(std::move(macro));
  selection_.SetActiveLayer(bones.id);
  hint_(QStringLiteral("Add Bone (A): drag out the first bone"));
}

bool LayerActions::DeleteLayer(Id layer, QString* why_not) {
  const LayerManager& layers = document_.Layers();
  if (layers.Find(layer) == nullptr) {
    *why_not = QStringLiteral("Pick a layer first");
    return false;
  }
  const int going = layers.EndOf(layer) - layers.IndexOf(layer);
  const bool is_last = layers.Count() - going < 1;
  if (is_last) {
    *why_not = QStringLiteral("A project keeps at least one layer");
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
  const LayerManager& layers = document_.Layers();
  const std::optional<int> to = layers.StepIndex(layer, steps);
  if (to.has_value()) {
    undo_.Push(std::make_unique<MoveLayerCommand>(
        document_, layer, layers.Find(layer)->parent, *to));
  }
}

bool LayerActions::PlaceLayer(Id layer, Id parent, int index,
                              QString* why_not) {
  const LayerManager& layers = document_.Layers();
  const Layer* found = layers.Find(layer);
  if (found == nullptr) {
    *why_not = QStringLiteral("Pick a layer first");
    return false;
  }
  const bool is_into_itself = parent == layer || layers.IsInside(parent, layer);
  if (is_into_itself) {
    *why_not = QStringLiteral("A layer can't go inside itself");
    return false;
  }
  // Try it on a copy, so a refused move leaves no undo step.
  LayerManager trial;
  for (const Layer& each : layers.All()) {
    trial.Insert(each, trial.Count());
  }
  if (!trial.Place(layer, parent, index)) {
    *why_not = QStringLiteral("%1 can't go there").arg(found->name);
    return false;
  }
  undo_.Push(
      std::make_unique<MoveLayerCommand>(document_, layer, parent, index));
  return true;
}

bool LayerActions::ImportPicture(const QString& path, QString* why_not) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    *why_not = QStringLiteral("Can't open %1").arg(path);
    return false;
  }
  if (file.size() > ImageStore::kMaxBytes) {
    *why_not = QStringLiteral("The picture is larger than 256 MB");
    return false;
  }
  ImageStore& images = document_.Images();
  const std::optional<QString> name = images.Add(file.readAll(), why_not);
  if (!name.has_value()) {
    return false;
  }
  const QSizeF picture = images.Find(*name)->size().toSizeF();
  const QSizeF canvas = document_.Canvas().size.toSizeF();
  const double scale = std::min({1.0, canvas.width() / picture.width(),
                                 canvas.height() / picture.height()});
  const QSizeF shown = picture * scale;
  const Layer layer{
      .id = document_.NewId(),
      .type = LayerType::kImage,
      .name = QFileInfo(path).completeBaseName(),
      .image = *name,
      .image_pos = QPointF((canvas.width() - shown.width()) / 2,
                           (canvas.height() - shown.height()) / 2),
      .image_scale = scale,
  };
  const Id active = selection_.ActiveLayer();
  auto command = std::make_unique<AddLayerCommand>(document_, layer, 0);
  command->setText(QStringLiteral("Import Picture"));
  undo_.Push(std::move(command));
  selection_.SetActiveLayer(active);
  hint_(QStringLiteral("Added %1 below your drawing").arg(layer.name));
  return true;
}

void LayerActions::RegisterCommands(CommandRegistry& commands) {
  commands.Add({
      .id = QStringLiteral("layer.add"),
      .title = QStringLiteral("New Layer"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N),
      .run = [this] { AddLayer(); },
  });
  commands.Add({
      .id = QStringLiteral("layer.add-bones"),
      .title = QStringLiteral("New Bone Layer"),
      .shortcut = {},
      .run = [this] { AddBoneLayer(); },
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

QString LayerActions::UnusedName(const QString& base) const {
  // The first free number. Of count + 1 numbers, at least one is free.
  const std::vector<Layer>& layers = document_.Layers().All();
  const int count = static_cast<int>(layers.size());
  for (int n = 1; n <= count + 1; ++n) {
    const QString name = QStringLiteral("%1 %2").arg(base).arg(n);
    const bool is_taken = std::ranges::any_of(
        layers, [&](const Layer& layer) { return layer.name == name; });
    if (!is_taken) {
      return name;
    }
  }
  return base;
}

}  // namespace snapper
