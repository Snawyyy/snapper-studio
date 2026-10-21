#pragma once

#include <memory>

class QWidget;

namespace snapper {

class CommandBox;
class CommandRegistry;
class MainWindow;
class SettingsManager;

// Owns every manager and the main window, and wires them together. The app
// and the UI driver each make one.
class Studio {
 public:
  Studio();
  ~Studio();
  Studio(const Studio&) = delete;
  Studio& operator=(const Studio&) = delete;

  // Shows the window as it was left last time.
  void Show();
  QWidget* Window() const;
  CommandRegistry& Commands() { return *commands_; }

 private:
  void RegisterAppCommands();
  void RestoreSession();
  void SaveSession();

  std::unique_ptr<SettingsManager> settings_;
  std::unique_ptr<CommandRegistry> commands_;
  std::unique_ptr<MainWindow> window_;
  CommandBox* command_box_ = nullptr;  // owned by the window
};

}  // namespace snapper
