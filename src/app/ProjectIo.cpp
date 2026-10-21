#include "app/ProjectIo.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>

#include <cstdio>

#include "app/CommandRegistry.h"
#include "app/ZipArchive.h"
#include "document/Document.h"
#include "edit/UndoManager.h"

namespace snapper {

namespace {

const QString kProjectEntry = QStringLiteral("project.json");
const QString kFileFilter = QStringLiteral("Snapper projects (*.snapper)");

}  // namespace

ProjectIo::ProjectIo(Document& document, UndoManager& undo, QWidget* window,
                     std::function<void(const QString&)> hint)
    : document_(document),
      undo_(undo),
      window_(window),
      hint_(std::move(hint)) {
  connect(&undo_, &UndoManager::HistoryChanged, this, [this] {
    has_changed_since_autosave_ = true;
    Q_EMIT TitleChanged();
  });
  autosave_timer_.setInterval(kAutosaveMs);
  connect(&autosave_timer_, &QTimer::timeout, this, &ProjectIo::Autosave);
  autosave_timer_.start();
  // Leaving the app for another one is a good moment to save a copy.
  connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
          [this](Qt::ApplicationState state) {
            if (state != Qt::ApplicationActive) {
              Autosave();
            }
          });
}

bool ProjectIo::WriteArchive(const QString& path,
                             const DocumentContents& contents, QString* error) {
  const QString temporary = path + QStringLiteral(".tmp");
  {
    ZipWriter zip;
    bool is_written =
        zip.Open(temporary, error) &&
        zip.Add(kProjectEntry, ProjectJson::Write(contents), error);
    for (auto it = contents.images.cbegin();
         is_written && it != contents.images.cend(); ++it) {
      is_written = zip.Add(it.key(), it.value(), error);
    }
    is_written = is_written && zip.Commit(error);
    if (!is_written) {
      QFile::remove(temporary);
      return false;
    }
  }
  // rename() swaps the file in one step, replacing the old one.
  const bool is_swapped = std::rename(QFile::encodeName(temporary).constData(),
                                      QFile::encodeName(path).constData()) == 0;
  if (!is_swapped) {
    QFile::remove(temporary);
    *error = QStringLiteral("Can't replace %1").arg(path);
    return false;
  }
  return true;
}

std::optional<LoadedProject> ProjectIo::ReadArchive(const QString& path,
                                                    QString* error) {
  ZipReader zip;
  if (!zip.Open(path, error)) {
    return std::nullopt;
  }
  const std::optional<QByteArray> json = zip.Read(kProjectEntry, error);
  if (!json.has_value()) {
    return std::nullopt;
  }
  std::optional<LoadedProject> loaded = ProjectJson::Read(*json, error);
  if (!loaded.has_value()) {
    return std::nullopt;
  }
  // Pictures sit beside project.json. A missing one leaves its layer
  // empty.
  for (const Layer& layer : loaded->contents.layers) {
    const bool needs_picture = layer.type == LayerType::kImage &&
                               !layer.image.isEmpty() &&
                               !loaded->contents.images.contains(layer.image);
    if (!needs_picture) {
      continue;
    }
    QString picture_error;
    const std::optional<QByteArray> bytes =
        zip.Read(layer.image, &picture_error);
    if (bytes.has_value()) {
      loaded->contents.images.insert(layer.image, *bytes);
    } else {
      loaded->warnings.append(
          QStringLiteral("The picture for %1 is missing").arg(layer.name));
    }
  }
  return loaded;
}

bool ProjectIo::SaveTo(const QString& path, QString* error) {
  if (!WriteArchive(path, document_.Contents(), error)) {
    return false;
  }
  path_ = path;
  undo_.MarkSaved();
  is_restored_unsaved_ = false;
  DropRecovery();
  Q_EMIT TitleChanged();
  hint_(QStringLiteral("Saved %1").arg(DisplayName()));
  return true;
}

bool ProjectIo::OpenFrom(const QString& path, QString* error) {
  const std::optional<LoadedProject> loaded = ReadArchive(path, error);
  if (!loaded.has_value()) {
    return false;
  }
  path_ = path;
  Loaded(*loaded);
  return true;
}

void ProjectIo::StartNew() {
  document_.ResetToNew();
  undo_.Clear();
  path_.clear();
  is_restored_unsaved_ = false;
  DropRecovery();
  Q_EMIT TitleChanged();
}

void ProjectIo::Loaded(const LoadedProject& loaded) {
  const int skipped = document_.Replace(loaded.contents);
  undo_.Clear();
  is_restored_unsaved_ = false;
  DropRecovery();
  Q_EMIT TitleChanged();
  QStringList warnings = loaded.warnings;
  if (skipped > 0) {
    warnings.append(
        QStringLiteral("%1 items were over the limits").arg(skipped));
  }
  if (warnings.isEmpty()) {
    hint_(QStringLiteral("Opened %1").arg(DisplayName()));
  } else {
    ShowError(QStringLiteral("Some of the project couldn't be read"),
              warnings.join(u'\n'));
  }
}

void ProjectIo::New() {
  if (ConfirmDiscard()) {
    StartNew();
  }
}

void ProjectIo::Open() {
  if (!ConfirmDiscard()) {
    return;
  }
  const QString folder =
      path_.isEmpty() ? QDir::homePath() : QFileInfo(path_).absolutePath();
  const QString path = QFileDialog::getOpenFileName(
      window_, QStringLiteral("Open Project"), folder, kFileFilter);
  if (path.isEmpty()) {
    return;
  }
  QString error;
  if (!OpenFrom(path, &error)) {
    ShowError(QStringLiteral("Couldn't open the project"), error);
  }
}

void ProjectIo::Save() {
  if (path_.isEmpty()) {
    SaveAs();
    return;
  }
  QString error;
  if (!SaveTo(path_, &error)) {
    ShowError(QStringLiteral("Couldn't save the project"), error);
  }
}

void ProjectIo::SaveAs() {
  const QString start = path_.isEmpty()
                            ? QDir::home().filePath(QStringLiteral("Untitled"))
                            : path_;
  QString path = QFileDialog::getSaveFileName(
      window_, QStringLiteral("Save Project"), start, kFileFilter);
  if (path.isEmpty()) {
    return;
  }
  if (!path.endsWith(QLatin1StringView(kSuffix))) {
    path += QLatin1StringView(kSuffix);
  }
  QString error;
  if (!SaveTo(path, &error)) {
    ShowError(QStringLiteral("Couldn't save the project"), error);
  }
}

bool ProjectIo::ConfirmDiscard() {
  const bool can_drop_quietly = !IsDirty() || !prompts_enabled_;
  if (can_drop_quietly) {
    return true;
  }
  const QMessageBox::StandardButton answer = QMessageBox::question(
      window_, QStringLiteral("Unsaved changes"),
      QStringLiteral("Save the changes to %1 first?").arg(DisplayName()),
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
      QMessageBox::Save);
  if (answer == QMessageBox::Save) {
    Save();
    return !IsDirty();
  }
  return answer == QMessageBox::Discard;
}

bool ProjectIo::IsDirty() const {
  return !undo_.IsSaved() || is_restored_unsaved_;
}

QString ProjectIo::DisplayName() const {
  return path_.isEmpty() ? QStringLiteral("Untitled")
                         : QFileInfo(path_).completeBaseName();
}

void ProjectIo::RegisterCommands(CommandRegistry& commands) {
  commands.Add({.id = QStringLiteral("file.new"),
                .title = QStringLiteral("New Project"),
                .shortcut = QKeySequence(Qt::CTRL | Qt::Key_N),
                .run = [this] { New(); }});
  commands.Add({.id = QStringLiteral("file.open"),
                .title = QStringLiteral("Open Project"),
                .shortcut = QKeySequence(Qt::CTRL | Qt::Key_O),
                .run = [this] { Open(); }});
  commands.Add({.id = QStringLiteral("file.save"),
                .title = QStringLiteral("Save"),
                .shortcut = QKeySequence(Qt::CTRL | Qt::Key_S),
                .run = [this] { Save(); }});
  commands.Add({.id = QStringLiteral("file.save-as"),
                .title = QStringLiteral("Save As"),
                .shortcut = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S),
                .run = [this] { SaveAs(); }});
  commands.Add({.id = QStringLiteral("file.restore-recovery"),
                .title = QStringLiteral("Restore Recovered Project"),
                .shortcut = {},
                .run = [this] { RestoreRecovery(); }});
}

QString ProjectIo::RecoveryFolder() {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         QStringLiteral("/recovery");
}

QString ProjectIo::RecoveryFile() {
  return RecoveryFolder() + QStringLiteral("/recovery.snapper");
}

QString ProjectIo::RecoveryInfoFile() {
  return RecoveryFolder() + QStringLiteral("/recovery.json");
}

// The copy goes next to the app's settings, never over the user's file.
void ProjectIo::Autosave() {
  const bool is_needed = IsDirty() && has_changed_since_autosave_;
  if (!is_needed) {
    return;
  }
  QDir().mkpath(RecoveryFolder());
  QString error;
  if (!WriteArchive(RecoveryFile(), document_.Contents(), &error)) {
    hint_(QStringLiteral("Autosave failed: %1").arg(error));
    return;
  }
  QFile info(RecoveryInfoFile());
  if (info.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    const QJsonObject json{{QStringLiteral("project"), path_}};
    info.write(QJsonDocument(json).toJson());
  }
  has_changed_since_autosave_ = false;
}

void ProjectIo::CheckForRecovery() {
  if (QFile::exists(RecoveryFile())) {
    hint_(QStringLiteral(
        "The last session closed with unsaved work. Run \"Restore Recovered "
        "Project\" from Commands (Ctrl+K) to get it back."));
  }
}

void ProjectIo::RestoreRecovery() {
  if (!QFile::exists(RecoveryFile())) {
    hint_(QStringLiteral("There is no recovered project"));
    return;
  }
  if (!ConfirmDiscard()) {
    return;
  }
  QString error;
  const std::optional<LoadedProject> loaded =
      ReadArchive(RecoveryFile(), &error);
  if (!loaded.has_value()) {
    ShowError(QStringLiteral("Couldn't restore the project"), error);
    return;
  }
  QFile info(RecoveryInfoFile());
  const QString original = info.open(QIODevice::ReadOnly)
                               ? QJsonDocument::fromJson(info.readAll())
                                     .object()
                                     .value(QStringLiteral("project"))
                                     .toString()
                               : QString();
  Loaded(*loaded);
  // Loading drops the recovery copy. Until saved, this is unsaved work.
  path_ = original;
  is_restored_unsaved_ = true;
  has_changed_since_autosave_ = true;
  Autosave();
  Q_EMIT TitleChanged();
  hint_(QStringLiteral("Restored %1. Save it to keep it.").arg(DisplayName()));
}

void ProjectIo::DropRecovery() {
  QFile::remove(RecoveryFile());
  QFile::remove(RecoveryInfoFile());
}

void ProjectIo::ShowError(const QString& title, const QString& text) {
  hint_(title);
  if (prompts_enabled_) {
    QMessageBox::warning(window_, title, text);
  } else {
    std::fprintf(stderr, "%s: %s\n", qPrintable(title), qPrintable(text));
  }
}

}  // namespace snapper
