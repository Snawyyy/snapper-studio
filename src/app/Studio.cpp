#include "app/Studio.h"

#include <QEvent>
#include <QUndoView>

#include <functional>

#include "app/CommandRegistry.h"
#include "app/ProjectIo.h"
#include "app/SettingsManager.h"
#include "app/StudyManager.h"
#include "app/ThemeManager.h"
#include "document/Document.h"
#include "edit/AnimationEditor.h"
#include "edit/BrushActions.h"
#include "edit/KeyActions.h"
#include "edit/LayerActions.h"
#include "edit/LookManager.h"
#include "edit/Playhead.h"
#include "edit/SelectionManager.h"
#include "edit/StyleActions.h"
#include "edit/ToolManager.h"
#include "edit/UndoManager.h"
#include "edit/tools/EyedropperTool.h"
#include "edit/tools/PenTool.h"
#include "edit/tools/SelectTool.h"
#include "edit/tools/ShapeTool.h"
#include "eval/FrameEvaluator.h"
#include "ui/BrushesPanel.h"
#include "ui/CanvasView.h"
#include "ui/ColorPopover.h"
#include "ui/CommandBox.h"
#include "ui/EyedropperOptions.h"
#include "ui/LayersPanel.h"
#include "ui/MainWindow.h"
#include "ui/PenOptions.h"
#include "ui/StudyNoteBox.h"
#include "ui/StylesPanel.h"
#include "ui/ToolOptionsPanel.h"

namespace snapper {

namespace {

// Asks its callback when the window is asked to close, while the panels
// are still open. By the time the app quits they are already closed. A
// false answer keeps the window open.
class CloseWatcher : public QObject {
 public:
  CloseWatcher(std::function<bool()> allow_close, QObject* parent)
      : QObject(parent), allow_close_(std::move(allow_close)) {}

  bool eventFilter(QObject* watched, QEvent* event) override {
    const bool is_close = event->type() == QEvent::Close;
    if (is_close && !allow_close_()) {
      event->ignore();
      return true;
    }
    return QObject::eventFilter(watched, event);
  }

 private:
  std::function<bool()> allow_close_;
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
      playhead_(std::make_unique<Playhead>(*document_)),
      frames_(std::make_unique<FrameEvaluator>(*document_, *playhead_)),
      animation_(std::make_unique<AnimationEditor>(*document_, *playhead_)),
      undo_(std::make_unique<UndoManager>()),
      selection_(std::make_unique<SelectionManager>(*document_)),
      looks_(std::make_unique<LookManager>(
          *document_, *selection_, *undo_,
          [this](const QString& text) { window_->ShowHint(text); })),
      tools_(std::make_unique<ToolManager>()) {
  document_->ResetToNew();
  looks_->RestoreNewShapes(settings_->NewShapeLook());
  QObject::connect(
      looks_.get(), &LookManager::NewShapesChanged, looks_.get(),
      [this] { settings_->SetNewShapeLook(looks_->NewShapes().look); });
  canvas_ = new CanvasView(*document_, *selection_, *frames_, *playhead_,
                           theme_->CurrentTheme());
  window_ = std::make_unique<MainWindow>(canvas_);
  layer_actions_ = std::make_unique<LayerActions>(
      *document_, *selection_, *undo_,
      [this](const QString& text) { window_->ShowHint(text); });
  CreateTools();
  CreatePanels();
  CreateTimeline();
  command_box_ = new CommandBox(commands_.get(), window_->Widget());
  RegisterAppCommands();
  undo_->RegisterCommands(*commands_);
  canvas_->RegisterCommands(*commands_);
  tools_->RegisterCommands(*commands_);
  selection_->RegisterCommands(*commands_, *undo_);
  looks_->RegisterCommands(*commands_);
  window_->RegisterCommands(*commands_);
  commands_->AttachShortcutsTo(window_->Widget());
  project_ = std::make_unique<ProjectIo>(
      *document_, *undo_, window_->Widget(),
      [this](const QString& text) { window_->ShowHint(text); });
  project_->RegisterCommands(*commands_);
  QObject::connect(project_.get(), &ProjectIo::TitleChanged, window_->Widget(),
                   [this] { UpdateTitle(); });
  UpdateTitle();
  CreateStudy();
  window_->Widget()->installEventFilter(
      new CloseWatcher([this] { return AllowClose(); }, window_->Widget()));
}

Studio::~Studio() = default;

void Studio::Show() {
  window_->Show();
  RestoreSession();
  project_->CheckForRecovery();
}

bool Studio::AllowClose() {
  if (!project_->ConfirmDiscard()) {
    return false;
  }
  project_->DropRecovery();
  SaveSession();
  return true;
}

void Studio::UpdateTitle() {
  QWidget* window = window_->Widget();
  window->setWindowTitle(
      QStringLiteral("%1[*] - Snapper Studio").arg(project_->DisplayName()));
  window->setWindowModified(project_->IsDirty());
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
      .looks = *looks_,
      .frames = *frames_,
      .animation = *animation_,
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
    tools_->Add(std::make_unique<ShapeTool>(context, kind));
  }

  auto eyedropper = std::make_unique<EyedropperTool>(context);
  EyedropperTool* dropper = eyedropper.get();
  auto* dropper_options = new EyedropperOptions(*dropper);
  options->AddPage(dropper->Info().id, dropper_options);
  tools_->Add(std::move(eyedropper));
  // I picks a color and Alt+I a whole style. Leaving the tool goes back to
  // colors, so I always means color.
  QObject::connect(tools_.get(), &ToolManager::ActiveChanged, dropper_options,
                   [dropper, dropper_options](const QString& id) {
                     if (id != dropper->Info().id) {
                       dropper->SetMode(EyedropperTool::Mode::kColor);
                       dropper_options->Refresh();
                     }
                   });
  pick_style_ = [this, dropper, dropper_options] {
    dropper->SetMode(EyedropperTool::Mode::kWholeStyle);
    dropper_options->Refresh();
    tools_->Activate(dropper->Info().id);
  };

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

  const auto hint = [this](const QString& text) { window_->ShowHint(text); };
  color_popover_ = new ColorPopover(window_->Widget());
  // Its colors belong to the project that was open.
  QObject::connect(document_.get(), &Document::Reset, color_popover_,
                   &ColorPopover::Close);
  style_actions_ =
      std::make_unique<StyleActions>(*document_, *looks_, *undo_, hint);
  style_actions_->RegisterCommands(*commands_);
  auto* styles = new StylesPanel(*document_, *looks_, *style_actions_,
                                 *color_popover_, theme_->CurrentTheme());
  window_->SetPanelContent(MainWindow::kStyles, styles);
  QObject::connect(styles, &StylesPanel::HighlightStyle, canvas_,
                   &CanvasView::HighlightStyle);

  brush_actions_ =
      std::make_unique<BrushActions>(*document_, *looks_, *undo_, hint);
  auto* brushes =
      new BrushesPanel(*document_, *brush_actions_, theme_->CurrentTheme());
  window_->SetPanelContent(MainWindow::kBrushes, brushes);
  commands_->Add({
      .id = QStringLiteral("brush.import"),
      .title = QStringLiteral("Import Brush"),
      .shortcut = {},
      .run = [brushes] { brushes->ImportBrush(); },
  });

  // Click a step to go back or forward to it.
  auto* history = new QUndoView(undo_->Stack());
  history->setObjectName(QStringLiteral("history"));
  history->setEmptyLabel(QStringLiteral("New project"));
  window_->SetPanelContent(MainWindow::kHistory, history);
}

void Studio::CreateTimeline() {
  key_actions_ = std::make_unique<KeyActions>(
      *document_, *selection_, *undo_, *playhead_, *frames_,
      [this](const QString& text) { window_->ShowHint(text); });
  key_actions_->RegisterCommands(*commands_);
  playhead_->RegisterCommands(
      *commands_,
      [this](int key, Qt::KeyboardModifiers modifiers) {
        return canvas_->hasFocus() && tools_->KeyPressed(key, modifiers);
      },
      [this] { return key_actions_->FramesIn(KeyRow::kAll); });
}

void Studio::CreateStudy() {
  study_ =
      std::make_unique<StudyManager>(window_->Widget(), *commands_, *undo_);
  study_->RegisterCommands(*commands_);
  note_box_ = new StudyNoteBox(*study_, window_->Widget());
  QObject::connect(study_.get(), &StudyManager::Hint, window_->Widget(),
                   [this](const QString& text) { window_->ShowHint(text); });
  QObject::connect(
      study_.get(), &StudyManager::RecordingChanged, window_->Widget(),
      [this](bool recording) { window_->ShowRecording(recording); });
  commands_->Add({
      .id = QStringLiteral("study.note"),
      .title = QStringLiteral("Add Study Note"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_N),
      .run =
          [this] {
            if (study_->IsRecording()) {
              note_box_->Open();
            } else {
              window_->ShowHint(QStringLiteral(
                  "Add Study Note: start a study recording first"));
            }
          },
  });
}

void Studio::RegisterAppCommands() {
  commands_->Add({
      .id = QStringLiteral("app.commands"),
      .title = QStringLiteral("Commands"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::Key_K),
      .run = [this] { command_box_->Open(); },
  });
  commands_->Add({
      .id = QStringLiteral("tool.eyedropper-style"),
      .title = QStringLiteral("Pick Whole Style"),
      .shortcut = QKeySequence(Qt::ALT | Qt::Key_I),
      .run = [this] { pick_style_(); },
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
