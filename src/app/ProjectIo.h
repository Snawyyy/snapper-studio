#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include <functional>
#include <optional>

#include "document/ProjectJson.h"

class QWidget;

namespace snapper {

class CommandRegistry;
class Document;
class UndoManager;

// Owns the project file: new, open, save, save as, autosave and recovery
// after a crash. Saving is safe: it writes a temporary file and only then
// swaps it in, so a crash mid-save never destroys the project.
class ProjectIo : public QObject {
  Q_OBJECT

 public:
  static constexpr int kAutosaveMs = 2 * 60 * 1000;
  static constexpr const char* kSuffix = ".snapper";

  ProjectIo(Document& document, UndoManager& undo, QWidget* window,
            std::function<void(const QString&)> hint);

  // Without dialogs, for the app's own use and for tests.
  bool SaveTo(const QString& path, QString* error);
  bool OpenFrom(const QString& path, QString* error);
  void StartNew();

  // The menu commands. They ask before dropping unsaved work.
  void New();
  void Open();
  void Save();
  void SaveAs();
  void RestoreRecovery();

  // True when it's fine to drop the current project: it's saved, or the
  // user saved it or chose to discard it just now.
  bool ConfirmDiscard();
  // Turns the questions off, for the UI driver, which can't answer them.
  void SetPromptsEnabled(bool enabled) { prompts_enabled_ = enabled; }

  // Writes the recovery copy now if there is unsaved work.
  void Autosave();
  // On start: says so when the last run left unsaved work behind.
  void CheckForRecovery();
  // On a clean exit: the recovery copy is no longer needed.
  void DropRecovery();

  const QString& Path() const { return path_; }
  bool IsDirty() const;
  // "Untitled" or the file name without its suffix.
  QString DisplayName() const;

  // New, Open, Save, Save As and Restore Recovered Project.
  void RegisterCommands(CommandRegistry& commands);

  // The archive alone, so tests can check what lands in a file.
  static bool WriteArchive(const QString& path,
                           const DocumentContents& contents, QString* error);
  static std::optional<LoadedProject> ReadArchive(const QString& path,
                                                  QString* error);

 Q_SIGNALS:
  // The name or the unsaved state changed.
  void TitleChanged();

 private:
  static QString RecoveryFolder();
  static QString RecoveryFile();
  static QString RecoveryInfoFile();
  void Loaded(const LoadedProject& loaded);
  void ShowError(const QString& title, const QString& text);

  Document& document_;
  UndoManager& undo_;
  QWidget* window_;
  std::function<void(const QString&)> hint_;
  QString path_;
  // Set after a recovery, which is unsaved work even with no undo steps.
  bool is_restored_unsaved_ = false;
  bool has_changed_since_autosave_ = false;
  bool prompts_enabled_ = true;
  QTimer autosave_timer_;
};

}  // namespace snapper
