#include "app/Studio.h"

#include <QEvent>

#include <functional>

#include "app/CommandRegistry.h"
#include "app/SettingsManager.h"
#include "ui/CommandBox.h"
#include "ui/MainWindow.h"

namespace snapper {

namespace {

// Calls back when its window is asked to close, while the panels are still
// open. By the time the app quits they are already closed.
class CloseWatcher : public QObject {
 public:
  CloseWatcher(std::function<void()> on_close, QObject* parent)
      : QObject(parent), on_close_(std::move(on_close)) {}

  bool eventFilter(QObject* watched, QEvent* event) override {
    const bool is_close = event->type() == QEvent::Close;
    if (is_close) {
      on_close_();
    }
    return QObject::eventFilter(watched, event);
  }

 private:
  std::function<void()> on_close_;
};

}  // namespace

Studio::Studio()
    : settings_(std::make_unique<SettingsManager>()),
      commands_(std::make_unique<CommandRegistry>()),
      window_(std::make_unique<MainWindow>()) {
  command_box_ = new CommandBox(commands_.get(), window_->Widget());
  RegisterAppCommands();
  window_->RegisterCommands(*commands_);
  commands_->AttachShortcutsTo(window_->Widget());
  window_->Widget()->installEventFilter(
      new CloseWatcher([this] { SaveSession(); }, window_->Widget()));
}

Studio::~Studio() = default;

void Studio::Show() {
  window_->Show();
  RestoreSession();
}

// A missing or unreadable layout keeps the default one.
void Studio::RestoreSession() {
  const QByteArray layout = settings_->PanelLayout();
  if (!layout.isEmpty()) {
    window_->RestoreLayout(layout);
  }
}

void Studio::SaveSession() { settings_->SetPanelLayout(window_->SaveLayout()); }

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
      // Closing the window, not quitting outright, saves the layout while
      // floating panels are still open, the same as the close button.
      .run = [this] { window_->Widget()->close(); },
  });
}

}  // namespace snapper
