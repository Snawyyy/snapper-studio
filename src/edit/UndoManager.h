#pragma once

#include <QObject>
#include <QString>

#include <memory>

class QUndoCommand;
class QUndoStack;

namespace snapper {

class CommandRegistry;

// Owns the undo history. Every edit to the document is a command pushed
// here, which runs it at once and keeps it for undo.
class UndoManager : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMaxSteps = 1000;

  explicit UndoManager(QObject* parent = nullptr);
  ~UndoManager() override;

  // Runs the command and keeps it. A null command is ignored.
  void Push(std::unique_ptr<QUndoCommand> command);
  void Undo();
  void Redo();
  void Clear();

  // Marks the current step as the saved one.
  void MarkSaved();
  bool IsSaved() const;

  bool CanUndo() const;
  bool CanRedo() const;
  QString UndoText() const;
  QString RedoText() const;

  // For the history panel, which shows the stack and steps through it.
  QUndoStack* Stack() const { return stack_.get(); }

  // Adds Undo and Redo, which name what they will change.
  void RegisterCommands(CommandRegistry& commands);

 Q_SIGNALS:
  void HistoryChanged();

 private:
  std::unique_ptr<QUndoStack> stack_;
};

}  // namespace snapper
