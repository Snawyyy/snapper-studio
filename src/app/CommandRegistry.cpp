#include "app/CommandRegistry.h"

#include <QAction>
#include <QWidget>

#include <algorithm>

namespace snapper {

namespace {

// Higher is a better match, 0 is no match. Title start beats word start
// beats anywhere, and letters in order ("shl" for "Show Layers") match last.
int MatchScore(const QString& title, const QString& query) {
  const qsizetype found = title.indexOf(query, 0, Qt::CaseInsensitive);
  if (found == 0) {
    return 4;
  }
  const bool is_word_start = found > 0 && title.at(found - 1) == u' ';
  if (is_word_start) {
    return 3;
  }
  if (found > 0) {
    return 2;
  }

  qsizetype next = 0;
  for (const QChar letter : query) {
    if (letter == u' ') {
      continue;
    }
    next = title.indexOf(letter, next, Qt::CaseInsensitive);
    if (next < 0) {
      return 0;
    }
    ++next;
  }
  return 1;
}

}  // namespace

CommandRegistry::CommandRegistry(QObject* parent) : QObject(parent) {}

bool CommandRegistry::Add(Command command) {
  const bool is_valid = !command.id.isEmpty() && command.run != nullptr;
  const bool is_full = Count() >= kMaxCommands;
  const bool is_taken = Find(command.id) != nullptr;
  if (!is_valid || is_full || is_taken) {
    return false;
  }
  commands_.push_back(std::move(command));
  AddShortcut(commands_.back());
  return true;
}

bool CommandRegistry::Run(const QString& id) {
  const Command* command = Find(id);
  if (command == nullptr) {
    return false;
  }
  command->run();
  Q_EMIT Ran(id);
  return true;
}

const Command* CommandRegistry::Find(const QString& id) const {
  const auto it = std::find_if(
      commands_.begin(), commands_.end(),
      [&id](const Command& command) { return command.id == id; });
  return it != commands_.end() ? &*it : nullptr;
}

std::vector<QString> CommandRegistry::Search(const QString& query) const {
  const QString trimmed = query.trimmed();
  const bool lists_everything = trimmed.isEmpty();
  struct Match {
    int score;
    int order;
    const Command* command;
  };
  std::vector<Match> matches;
  for (int i = 0; i < Count(); ++i) {
    const Command& command = commands_[static_cast<size_t>(i)];
    const int score =
        lists_everything ? 1 : MatchScore(command.title, trimmed);
    if (score > 0) {
      matches.push_back({score, i, &command});
    }
  }

  std::stable_sort(matches.begin(), matches.end(),
                   [](const Match& a, const Match& b) {
                     return a.score > b.score;
                   });
  std::vector<QString> ids;
  const size_t count =
      std::min(matches.size(), static_cast<size_t>(kMaxSearchResults));
  for (size_t i = 0; i < count; ++i) {
    ids.push_back(matches[i].command->id);
  }
  return ids;
}

void CommandRegistry::AttachShortcutsTo(QWidget* host) {
  shortcut_host_ = host;
  for (const Command& command : commands_) {
    AddShortcut(command);
  }
}

// Each shortcut is a QAction on the host window. It is app wide so it also
// fires from floating panels.
void CommandRegistry::AddShortcut(const Command& command) {
  const bool can_add =
      shortcut_host_ != nullptr && !command.shortcut.isEmpty();
  if (!can_add) {
    return;
  }
  auto* action = new QAction(command.title, shortcut_host_);
  action->setShortcut(command.shortcut);
  action->setShortcutContext(Qt::ApplicationShortcut);
  const QString id = command.id;
  connect(action, &QAction::triggered, this, [this, id] { Run(id); });
  shortcut_host_->addAction(action);
}

}  // namespace snapper
