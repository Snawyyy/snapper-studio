#pragma once

#include <functional>
#include <memory>

class QWidget;

namespace snapper {

class BrushActions;
class CanvasView;
class ColorPopover;
class CommandBox;
class CommandRegistry;
class AnimationEditor;
class Document;
class FrameEvaluator;
class KeyActions;
class LayerActions;
class LookManager;
class MainWindow;
class Playhead;
class ProjectIo;
class SelectionManager;
class SettingsManager;
class StudyManager;
class StudyNoteBox;
class StyleActions;
class ThemeManager;
class ToolManager;
class UndoManager;

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
  Document& Doc() { return *document_; }
  UndoManager& Undo() { return *undo_; }
  ToolManager& Tools() { return *tools_; }
  ProjectIo& Project() { return *project_; }
  LayerActions& Layers() { return *layer_actions_; }

 private:
  void RegisterAppCommands();
  void CreateTools();
  void CreatePanels();
  void CreateTimeline();
  void CreateStudy();
  void RestoreSession();
  void SaveSession();
  // Asks about unsaved work. False keeps the window open.
  bool AllowClose();
  void UpdateTitle();

  std::unique_ptr<ThemeManager> theme_;
  std::unique_ptr<SettingsManager> settings_;
  std::unique_ptr<CommandRegistry> commands_;
  std::unique_ptr<Document> document_;
  // Made right after the document, so poses are dropped before anything
  // else hears of a change.
  std::unique_ptr<Playhead> playhead_;
  std::unique_ptr<FrameEvaluator> frames_;
  std::unique_ptr<AnimationEditor> animation_;
  std::unique_ptr<UndoManager> undo_;
  std::unique_ptr<SelectionManager> selection_;
  std::unique_ptr<LookManager> looks_;
  std::unique_ptr<ToolManager> tools_;
  std::unique_ptr<LayerActions> layer_actions_;
  std::unique_ptr<StyleActions> style_actions_;
  std::unique_ptr<BrushActions> brush_actions_;
  std::unique_ptr<KeyActions> key_actions_;
  std::unique_ptr<ProjectIo> project_;
  std::unique_ptr<StudyManager> study_;
  std::unique_ptr<MainWindow> window_;
  // Both owned by the window.
  CanvasView* canvas_ = nullptr;
  CommandBox* command_box_ = nullptr;
  StudyNoteBox* note_box_ = nullptr;
  ColorPopover* color_popover_ = nullptr;
  // Switches to the eyedropper, picking whole styles.
  std::function<void()> pick_style_;
};

}  // namespace snapper
