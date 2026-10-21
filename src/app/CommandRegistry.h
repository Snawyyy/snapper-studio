#pragma once

#include <QKeySequence>
#include <QObject>
#include <QString>

#include <functional>
#include <vector>

class QAction;
class QWidget;

namespace snapper {

// One user action, findable by name and runnable from anywhere.
struct Command {
  QString id;     // stable, never shown: "panel.layers.toggle"
  QString title;  // shown in menus and the command box: "Show Layers"
  QKeySequence shortcut;
  std::function<void()> run;
};

// Owns every user action. Menus, the Ctrl+K command box and shortcut
// rebinding all read from here, and every run is announced through Ran()
// so usability recording can log it.
class CommandRegistry : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMaxCommands = 1024;
  static constexpr int kMaxSearchResults = 50;

  explicit CommandRegistry(QObject* parent = nullptr);

  // Fails on an empty or repeated id, a missing run function, or a full
  // registry.
  bool Add(Command command);

  // Returns false when no command has this id.
  bool Run(const QString& id);

  const Command* Find(const QString& id) const;
  int Count() const { return static_cast<int>(commands_.size()); }

  // Ids of commands matching the query, best match first. An empty query
  // lists every command in the order it was added.
  std::vector<QString> Search(const QString& query) const;

  // Makes every shortcut work anywhere in the app, including for commands
  // added later. Call once.
  void AttachShortcutsTo(QWidget* host);

 Q_SIGNALS:
  void Ran(const QString& id);

 private:
  void AddShortcut(const Command& command);

  std::vector<Command> commands_;
  QWidget* shortcut_host_ = nullptr;
};

}  // namespace snapper
