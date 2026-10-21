#include "edit/UndoManager.h"

#include <QUndoStack>

#include "app/CommandRegistry.h"

namespace snapper {

UndoManager::UndoManager(QObject* parent)
    : QObject(parent), stack_(std::make_unique<QUndoStack>()) {
  stack_->setUndoLimit(kMaxSteps);
  connect(stack_.get(), &QUndoStack::indexChanged, this,
          &UndoManager::HistoryChanged);
  connect(stack_.get(), &QUndoStack::cleanChanged, this,
          &UndoManager::HistoryChanged);
}

UndoManager::~UndoManager() = default;

void UndoManager::Push(std::unique_ptr<QUndoCommand> command) {
  if (command != nullptr) {
    stack_->push(command.release());
  }
}

void UndoManager::Undo() { stack_->undo(); }
void UndoManager::Redo() { stack_->redo(); }
void UndoManager::Clear() { stack_->clear(); }
void UndoManager::MarkSaved() { stack_->setClean(); }
bool UndoManager::IsSaved() const { return stack_->isClean(); }
bool UndoManager::CanUndo() const { return stack_->canUndo(); }
bool UndoManager::CanRedo() const { return stack_->canRedo(); }
QString UndoManager::UndoText() const { return stack_->undoText(); }
QString UndoManager::RedoText() const { return stack_->redoText(); }

void UndoManager::RegisterCommands(CommandRegistry& commands) {
  commands.Add({
      .id = QStringLiteral("edit.undo"),
      .title = QStringLiteral("Undo"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::Key_Z),
      .run = [this] { Undo(); },
  });
  commands.Add({
      .id = QStringLiteral("edit.redo"),
      .title = QStringLiteral("Redo"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z),
      .run = [this] { Redo(); },
  });
}

}  // namespace snapper
