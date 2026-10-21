#pragma once

#include <memory>

class QWidget;

namespace snapper {

class CommandBox;
class CommandRegistry;
class MainWindow;

// Owns every manager and the main window, and wires them together. The app
// and the UI driver each make one.
class Studio {
 public:
  Studio();
  ~Studio();
  Studio(const Studio&) = delete;
  Studio& operator=(const Studio&) = delete;

  void Show();
  QWidget* Window() const;
  CommandRegistry& Commands() { return *commands_; }

 private:
  void RegisterAppCommands();

  std::unique_ptr<CommandRegistry> commands_;
  std::unique_ptr<MainWindow> window_;
  CommandBox* command_box_ = nullptr;  // owned by the window
};

}  // namespace snapper
