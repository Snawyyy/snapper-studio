#pragma once

#include <kddockwidgets/qtwidgets/views/MainWindow.h>

#include <array>

namespace KDDockWidgets::QtWidgets {
class DockWidget;
}

namespace snapper {

// The app window: the canvas sits fixed in the middle and takes all extra
// space, and every other panel is a dock the user can move, tab, float and
// close. Panels start empty until their milestone fills them.
class MainWindow : public KDDockWidgets::QtWidgets::MainWindow {
  Q_OBJECT

 public:
  static constexpr int kPanelCount = 7;

  explicit MainWindow(QWidget* parent = nullptr);

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

  std::array<KDDockWidgets::QtWidgets::DockWidget*, kPanelCount> panels_{};
};

}  // namespace snapper
