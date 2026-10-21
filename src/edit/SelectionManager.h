#pragma once

#include <QObject>

#include "document/Ids.h"

namespace snapper {

class Document;

// Owns what is selected. For now that is the active layer, the one new
// drawing goes on.
class SelectionManager : public QObject {
  Q_OBJECT

 public:
  explicit SelectionManager(Document& document, QObject* parent = nullptr);

  Id ActiveLayer() const { return active_layer_; }
  // Returns false when no layer has this id.
  bool SetActiveLayer(Id layer);

 Q_SIGNALS:
  void ActiveLayerChanged(Id layer);

 private:
  // Keeps the active layer pointing at a layer that exists, falling back
  // to the top one.
  void KeepActiveLayerValid();

  Document& document_;
  Id active_layer_ = kNoId;
};

}  // namespace snapper
