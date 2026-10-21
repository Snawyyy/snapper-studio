#pragma once

#include <QSet>
#include <QUndoCommand>

#include <memory>

#include "document/Ids.h"

namespace snapper {

class SelectionManager;

// Changes which points are selected, so undo steps back through
// selections too.
class SelectPointsCommand : public QUndoCommand {
 public:
  SelectPointsCommand(SelectionManager& selection, QSet<Id> before,
                      QSet<Id> after);

  void redo() override;
  void undo() override;

 private:
  SelectionManager& selection_;
  QSet<Id> before_;
  QSet<Id> after_;
};

// Null when the selection would not change.
std::unique_ptr<QUndoCommand> MakeSelectPoints(SelectionManager& selection,
                                               const QSet<Id>& after);

}  // namespace snapper
