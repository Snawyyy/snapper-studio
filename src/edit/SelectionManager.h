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
// the select tool edits, the selected points on it, and the selected bones
// of the bone layer it is or sits in. Point and bone selection changes go
// through undo.
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

  // The bone layer the bone tools work on: the active layer, or the
  // nearest bone layer it sits in. kNoId for none.
  Id BoneLayer() const;
  const QSet<Id>& Bones() const { return bones_; }
  bool IsBoneSelected(Id bone) const { return bones_.contains(bone); }
  // Sets the bone selection at once. Edits go through SelectBonesCommand.
  void SetBones(const QSet<Id>& bones);

  // Select All, Select None, Invert Selection and Delete.
  void RegisterCommands(CommandRegistry& commands, UndoManager& undo);

 Q_SIGNALS:
  void ActiveLayerChanged(Id layer);
  void PointsChanged();
  void BonesChanged();

 private:
  // Keeps the active layer pointing at a layer that exists, falling back
  // to the top one.
  void KeepActiveLayerValid();
  // Drops points that no longer exist.
  void Prune();
  // Drops bones that no longer exist or are on another bone layer.
  void PruneBones();
  void DeleteSelected(UndoManager& undo);

  Document& document_;
  Id active_layer_ = kNoId;
  QSet<Id> points_;
  QSet<Id> bones_;
};

}  // namespace snapper
