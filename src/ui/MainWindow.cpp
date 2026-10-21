#include "ui/MainWindow.h"

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <kddockwidgets/qtwidgets/views/MainWindow.h>

#include <QWidget>

#include "app/CommandRegistry.h"

namespace snapper {

namespace {

struct PanelInfo {
  const char* id;
  const char* title;
};

// Ids are saved in the layout, so they must never change.
constexpr std::array<PanelInfo, MainWindow::kPanelCount> kPanelInfo = {{
    {"tool-options", "Tool Options"},
    {"history", "History"},
    {"layers", "Layers"},
    {"actions", "Actions"},
    {"styles", "Styles"},
    {"brushes", "Brushes"},
    {"timeline", "Timeline"},
}};

const KDDockWidgets::MainWindowOptions kWindowOptions =
    KDDockWidgets::MainWindowOptions(
        KDDockWidgets::MainWindowOption_HasCentralWidget) |
    KDDockWidgets::MainWindowOption_CentralWidgetGetsAllExtraSpace;

constexpr int kPanelMinWidth = 220;
constexpr int kPanelMinHeight = 120;

}  // namespace

MainWindow::MainWindow()
    : window_(std::make_unique<KDDockWidgets::QtWidgets::MainWindow>(
          QStringLiteral("main-window"), kWindowOptions)) {
  window_->setWindowTitle(QStringLiteral("Snapper Studio"));
  window_->resize(1280, 800);
  auto* canvas = new QWidget();
  canvas->setObjectName(QStringLiteral("canvas"));
  canvas->setAttribute(Qt::WA_StyledBackground);
  window_->setPersistentCentralWidget(canvas);
  CreatePanels();
  ArrangeDefaultLayout();
}

MainWindow::~MainWindow() = default;

QWidget* MainWindow::Widget() const { return window_.get(); }

void MainWindow::Show() { window_->show(); }

void MainWindow::RegisterCommands(CommandRegistry& commands) {
  for (int i = 0; i < kPanelCount; ++i) {
    const PanelInfo& info = kPanelInfo[static_cast<size_t>(i)];
    commands.Add({
        .id = QStringLiteral("panel.%1.show").arg(QLatin1StringView(info.id)),
        .title = QStringLiteral("Show %1").arg(QLatin1StringView(info.title)),
        .shortcut = {},
        .run = [this, i] { ShowPanel(i); },
    });
  }
}

void MainWindow::ShowPanel(int panel) {
  KDDockWidgets::QtWidgets::DockWidget* dock =
      panels_[static_cast<size_t>(panel)];
  if (!dock->isOpen()) {
    dock->open();
  }
  dock->setAsCurrentTab();
  dock->raise();
}

void MainWindow::CreatePanels() {
  for (int i = 0; i < kPanelCount; ++i) {
    const PanelInfo& info = kPanelInfo[static_cast<size_t>(i)];
    auto* dock = new KDDockWidgets::QtWidgets::DockWidget(
        QString::fromLatin1(info.id));
    dock->setTitle(QString::fromLatin1(info.title));
    auto* content = new QWidget();
    content->setMinimumSize(kPanelMinWidth, kPanelMinHeight);
    dock->setWidget(content);
    panels_[static_cast<size_t>(i)] = dock;
  }
}

// Moho style: tools on the left, layers and styles on the right, the
// timeline across the bottom.
void MainWindow::ArrangeDefaultLayout() {
  using KDDockWidgets::Location_OnBottom;
  using KDDockWidgets::Location_OnLeft;
  using KDDockWidgets::Location_OnRight;
  const KDDockWidgets::InitialOption side_size(QSize(260, 0));
  const KDDockWidgets::InitialOption timeline_size(QSize(0, 180));

  window_->addDockWidget(panels_[kToolOptions], Location_OnLeft, nullptr, side_size);
  window_->addDockWidget(panels_[kLayers], Location_OnRight, nullptr, side_size);
  panels_[kLayers]->addDockWidgetAsTab(panels_[kActions]);
  window_->addDockWidget(panels_[kStyles], Location_OnBottom, panels_[kLayers]);
  panels_[kStyles]->addDockWidgetAsTab(panels_[kBrushes]);
  window_->addDockWidget(panels_[kTimeline], Location_OnBottom, nullptr,
                         timeline_size);
  panels_[kToolOptions]->addDockWidgetAsTab(panels_[kHistory]);
  panels_[kLayers]->setAsCurrentTab();
  panels_[kStyles]->setAsCurrentTab();
  panels_[kToolOptions]->setAsCurrentTab();
}

}  // namespace snapper
