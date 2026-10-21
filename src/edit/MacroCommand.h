#pragma once

#include <QUndoCommand>

#include <memory>
#include <vector>

namespace snapper {

// Several commands as one undo step: redo runs them in order, undo runs
// them backwards.
class MacroCommand : public QUndoCommand {
 public:
  explicit MacroCommand(const QString& text) : QUndoCommand(text) {}

  // Null commands are skipped.
  void Add(std::unique_ptr<QUndoCommand> command) {
    if (command != nullptr) {
      steps_.push_back(std::move(command));
    }
  }
  bool IsEmpty() const { return steps_.empty(); }

  void redo() override {
    for (const std::unique_ptr<QUndoCommand>& step : steps_) {
      step->redo();
    }
  }

  void undo() override {
    for (auto it = steps_.rbegin(); it != steps_.rend(); ++it) {
      (*it)->undo();
    }
  }

 private:
  std::vector<std::unique_ptr<QUndoCommand>> steps_;
};

}  // namespace snapper
