#include "app/Studio.h"

#include <QEvent>

#include <functional>

#include "app/CommandRegistry.h"
#include "app/SettingsManager.h"
#include "app/ThemeManager.h"
#include "document/Document.h"
#include "edit/LayerActions.h"
#include "edit/SelectionManager.h"
#include "edit/ToolManager.h"
#include "edit/UndoManager.h"
#include "edit/tools/PenTool.h"
#include "edit/tools/SelectTool.h"
#include "edit/tools/ShapeTool.h"
#include "ui/CanvasView.h"
#include "ui/CommandBox.h"
#include "ui/LayersPanel.h"
#include "ui/MainWindow.h"
#include "ui/PenOptions.h"
#include "ui/ShapeOptions.h"
#include "ui/ToolOptionsPanel.h"

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

// The theme was checked when the app started, so a failed load here only
// leaves the canvas with default colors.
std::unique_ptr<ThemeManager> LoadTheme() {
  auto theme = std::make_unique<ThemeManager>();
  theme->LoadFromFile(QStringLiteral(":/theme/theme.json"));
  return theme;
}

}  // namespace

Studio::Studio()
    : theme_(LoadTheme()),
      settings_(std::make_unique<SettingsManager>()),
      commands_(std::make_unique<CommandRegistry>()),
      document_(std::make_unique<Document>()),
      undo_(std::make_unique<UndoManager>()),
      selection_(std::make_unique<SelectionManager>(*document_)),
      tools_(std::make_unique<ToolManager>()) {
  document_->ResetToNew();
  canvas_ = new CanvasView(*document_, *selection_, theme_->CurrentTheme());
  window_ = std::make_unique<MainWindow>(canvas_);
  layer_actions_ = std::make_unique<LayerActions>(
      *document_, *selection_, *undo_,
      [this](const QString& text) { window_->ShowHint(text); });
  CreateTools();
  CreatePanels();
  command_box_ = new CommandBox(commands_.get(), window_->Widget());
  RegisterAppCommands();
  undo_->RegisterCommands(*commands_);
  canvas_->RegisterCommands(*commands_);
  tools_->RegisterCommands(*commands_);
  selection_->RegisterCommands(*commands_, *undo_);
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

void Studio::CreateTools() {
  const ToolContext context{
      .document = *document_,
      .undo = *undo_,
      .selection = *selection_,
      .theme = theme_->CurrentTheme(),
      .hint = [this](const QString& text) { window_->ShowHint(text); },
  };
  auto* options = new ToolOptionsPanel();

  auto pen = std::make_unique<PenTool>(context);
  pen->SetSettings(settings_->Pen());
  options->AddPage(pen->Info().id,
                   new PenOptions(*pen, [this](const PenSettings& settings) {
                     settings_->SetPen(settings);
                   }));
  tools_->Add(std::move(pen));
  tools_->Add(std::make_unique<SelectTool>(context));

  for (const ShapeTool::Kind kind :
       {ShapeTool::Kind::kRectangle, ShapeTool::Kind::kEllipse}) {
    auto shape = std::make_unique<ShapeTool>(context, kind);
    const QString id = shape->Info().id;
    shape->SetSettings(settings_->ShapeTool(id));
    options->AddPage(
        id, new ShapeOptions(*shape, [this, id](const ShapeToolSettings& s) {
          settings_->SetShapeTool(id, s);
        }));
    tools_->Add(std::move(shape));
  }

  window_->SetPanelContent(MainWindow::kToolOptions, options);
  options->ShowTool(tools_->ActiveId());
  QObject::connect(tools_.get(), &ToolManager::ActiveChanged, options,
                   &ToolOptionsPanel::ShowTool);
  QObject::connect(tools_.get(), &ToolManager::Hint, window_->Widget(),
                   [this](const QString& text) { window_->ShowHint(text); });
  canvas_->SetTools(tools_.get());
}

void Studio::CreatePanels() {
  auto* layers = new LayersPanel(*document_, *selection_, *layer_actions_,
                                 theme_->CurrentTheme());
  layers->RegisterCommands(*commands_);
  window_->SetPanelContent(MainWindow::kLayers, layers);
  layer_actions_->RegisterCommands(*commands_);
}

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
