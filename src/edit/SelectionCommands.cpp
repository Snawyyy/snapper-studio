#include "edit/SelectionCommands.h"

#include "edit/SelectionManager.h"

namespace snapper {

SelectPointsCommand::SelectPointsCommand(SelectionManager& selection,
                                         QSet<Id> before, QSet<Id> after)
    : QUndoCommand(after.isEmpty() ? QStringLiteral("Select None")
                                   : QStringLiteral("Select Points")),
      selection_(selection),
      before_(std::move(before)),
      after_(std::move(after)) {}

void SelectPointsCommand::redo() { selection_.SetPoints(after_); }
void SelectPointsCommand::undo() { selection_.SetPoints(before_); }

std::unique_ptr<QUndoCommand> MakeSelectPoints(SelectionManager& selection,
                                               const QSet<Id>& after) {
  if (after == selection.Points()) {
    return nullptr;
  }
  return std::make_unique<SelectPointsCommand>(selection, selection.Points(),
                                               after);
}

}  // namespace snapper
