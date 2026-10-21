#pragma once

#include <QObject>
#include <QRectF>
#include <QSet>

#include <optional>

#include "document/Ids.h"

namespace snapper {

class CommandRegistry;
class Document;
class UndoManager;

// Owns what is selected: the active layer, the one new drawing goes on and
// the select tool edits, and the selected points on it. Point selection
// changes go through undo.
class SelectionManager : public QObject {
  Q_OBJECT

 public:
  explicit SelectionManager(Document& document, QObject* parent = nullptr);

  Id ActiveLayer() const { return active_layer_; }
  // Returns false when no layer has this id. Switching layers drops the
  // point selection.
  bool SetActiveLayer(Id layer);

  const QSet<Id>& Points() const { return points_; }
  bool IsSelected(Id point) const { return points_.contains(point); }
  // Sets the selection at once. Edits go through SelectPointsCommand.
  void SetPoints(const QSet<Id>& points);
  // The box round the selected points, if any are selected.
  std::optional<QRectF> Bounds() const;
  // Every point on the active layer.
  QSet<Id> AllPointsOnActiveLayer() const;

  // Select All, Select None, Invert Selection and Delete.
  void RegisterCommands(CommandRegistry& commands, UndoManager& undo);

 Q_SIGNALS:
  void ActiveLayerChanged(Id layer);
  void PointsChanged();

 private:
  // Keeps the active layer pointing at a layer that exists, falling back
  // to the top one.
  void KeepActiveLayerValid();
  // Drops points that no longer exist.
  void Prune();
  void DeleteSelected(UndoManager& undo);

  Document& document_;
  Id active_layer_ = kNoId;
  QSet<Id> points_;
};

}  // namespace snapper
