#pragma once

#include <array>
#include <memory>

class QWidget;

namespace KDDockWidgets::QtWidgets {
class DockWidget;
class MainWindow;
}  // namespace KDDockWidgets::QtWidgets

namespace snapper {

// The app window: the canvas sits fixed in the middle and takes all extra
// space, and every other panel is a dock the user can move, tab, float and
// close. Panels start empty until their milestone fills them.
//
// Owns a plain docking-library window instead of subclassing it: the
// library type-checks its windows at runtime, and our code is built
// without that type information.
class MainWindow {
 public:
  static constexpr int kPanelCount = 7;

  MainWindow();
  ~MainWindow();
  MainWindow(const MainWindow&) = delete;
  MainWindow& operator=(const MainWindow&) = delete;

  QWidget* Widget() const;
  void Show();

 private:
  enum Panel {
    kToolOptions,
    kHistory,
    kLayers,
    kActions,
    kStyles,
    kBrushes,
    kTimeline,
  };

  void CreatePanels();
  void ArrangeDefaultLayout();

  std::unique_ptr<KDDockWidgets::QtWidgets::MainWindow> window_;
  std::array<KDDockWidgets::QtWidgets::DockWidget*, kPanelCount> panels_{};
};

}  // namespace snapper
