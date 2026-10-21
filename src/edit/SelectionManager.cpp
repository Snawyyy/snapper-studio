#include "edit/SelectionManager.h"

#include <algorithm>

#include "app/CommandRegistry.h"
#include "document/Document.h"
#include "edit/MacroCommand.h"
#include "edit/SelectionCommands.h"
#include "edit/ShapeCommands.h"
#include "edit/UndoManager.h"

namespace snapper {

SelectionManager::SelectionManager(Document& document, QObject* parent)
    : QObject(parent), document_(document) {
  connect(&document_.Layers(), &LayerManager::ListChanged, this,
          &SelectionManager::KeepActiveLayerValid);
  connect(&document_.Shapes(), &ShapeManager::LayerContentChanged, this,
          &SelectionManager::Prune);
  connect(&document_, &Document::Reset, this, [this] {
    active_layer_ = kNoId;
    points_.clear();
    KeepActiveLayerValid();
    Q_EMIT PointsChanged();
  });
  KeepActiveLayerValid();
}

bool SelectionManager::SetActiveLayer(Id layer) {
  if (document_.Layers().Find(layer) == nullptr) {
    return false;
  }
  if (layer != active_layer_) {
    active_layer_ = layer;
    SetPoints({});
    Q_EMIT ActiveLayerChanged(layer);
  }
  return true;
}

void SelectionManager::SetPoints(const QSet<Id>& points) {
  if (points == points_) {
    return;
  }
  points_ = points;
  Q_EMIT PointsChanged();
}

// Built from corners by hand: QRectF::united skips rects of zero size.
std::optional<QRectF> SelectionManager::Bounds() const {
  bool has_any = false;
  QPointF low;
  QPointF high;
  for (const Id point : points_) {
    const std::optional<QPointF> pos = document_.Shapes().PositionOf(point);
    if (!pos.has_value()) {
      continue;
    }
    low = has_any ? QPointF(std::min(low.x(), pos->x()),
                            std::min(low.y(), pos->y()))
                  : *pos;
    high = has_any ? QPointF(std::max(high.x(), pos->x()),
                             std::max(high.y(), pos->y()))
                   : *pos;
    has_any = true;
  }
  if (!has_any) {
    return std::nullopt;
  }
  return QRectF(low, high);
}

QSet<Id> SelectionManager::AllPointsOnActiveLayer() const {
  QSet<Id> all;
  for (const Shape& shape : document_.Shapes().All()) {
    if (shape.layer != active_layer_) {
      continue;
    }
    for (const Point& point : shape.points) {
      all.insert(point.id);
    }
  }
  return all;
}

void SelectionManager::RegisterCommands(CommandRegistry& commands,
                                        UndoManager& undo) {
  commands.Add({
      .id = QStringLiteral("select.all"),
      .title = QStringLiteral("Select All"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::Key_A),
      .run =
          [this, &undo] {
            undo.Push(MakeSelectPoints(*this, AllPointsOnActiveLayer()));
          },
  });
  commands.Add({
      .id = QStringLiteral("select.none"),
      .title = QStringLiteral("Select None"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A),
      .run = [this, &undo] { undo.Push(MakeSelectPoints(*this, {})); },
  });
  commands.Add({
      .id = QStringLiteral("select.invert"),
      .title = QStringLiteral("Invert Selection"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::Key_I),
      .run =
          [this, &undo] {
            undo.Push(MakeSelectPoints(
                *this, AllPointsOnActiveLayer().subtract(points_)));
          },
  });
  commands.Add({
      .id = QStringLiteral("edit.delete"),
      .title = QStringLiteral("Delete"),
      .shortcut = QKeySequence(Qt::Key_Delete),
      .run = [this, &undo] { DeleteSelected(undo); },
  });
}

// Clears the selection first, so undo puts the points back before it
// selects them again.
void SelectionManager::DeleteSelected(UndoManager& undo) {
  std::unique_ptr<QUndoCommand> remove =
      MakeRemovePoints(document_, {points_.cbegin(), points_.cend()});
  if (remove == nullptr) {
    return;
  }
  auto macro = std::make_unique<MacroCommand>(remove->text());
  macro->Add(MakeSelectPoints(*this, {}));
  macro->Add(std::move(remove));
  undo.Push(std::move(macro));
}

void SelectionManager::KeepActiveLayerValid() {
  const LayerManager& layers = document_.Layers();
  const bool is_valid = layers.Find(active_layer_) != nullptr;
  if (is_valid) {
    return;
  }
  active_layer_ = layers.Count() == 0 ? kNoId : layers.All().back().id;
  SetPoints({});
  Q_EMIT ActiveLayerChanged(active_layer_);
}

void SelectionManager::Prune() {
  QSet<Id> alive = points_;
  alive.removeIf([this](Id point) {
    return document_.Shapes().ShapeOfPoint(point) == kNoId;
  });
  SetPoints(alive);
}

}  // namespace snapper
