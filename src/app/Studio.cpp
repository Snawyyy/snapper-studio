#include "app/Studio.h"

#include <QApplication>

#include "app/CommandRegistry.h"
#include "ui/CommandBox.h"
#include "ui/MainWindow.h"

namespace snapper {

Studio::Studio()
    : commands_(std::make_unique<CommandRegistry>()),
      window_(std::make_unique<MainWindow>()) {
  command_box_ = new CommandBox(commands_.get(), window_->Widget());
  RegisterAppCommands();
  window_->RegisterCommands(*commands_);
  commands_->AttachShortcutsTo(window_->Widget());
}

Studio::~Studio() = default;

void Studio::Show() { window_->Show(); }

QWidget* Studio::Window() const { return window_->Widget(); }

void Studio::RegisterAppCommands() {
  commands_->Add({
      .id = QStringLiteral("app.commands"),
      .title = QStringLiteral("Commands"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::Key_K),
      .run = [this] { command_box_->Open(); },
  });
  commands_->Add({
      .id = QStringLiteral("app.quit"),
      .title = QStringLiteral("Quit"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::Key_Q),
      .run = [] { QApplication::quit(); },
  });
}

}  // namespace snapper
