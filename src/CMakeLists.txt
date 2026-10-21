target_sources(snapper_core PRIVATE
  app/CommandRegistry.cpp app/CommandRegistry.h
  app/ProjectIo.cpp app/ProjectIo.h
  app/SettingsManager.cpp app/SettingsManager.h
  app/Startup.cpp app/Startup.h
  app/Studio.cpp app/Studio.h
  app/Theme.h
  app/StudyManager.cpp app/StudyManager.h
  app/ThemeManager.cpp app/ThemeManager.h
  app/ZipArchive.cpp app/ZipArchive.h
  document/BrushManager.cpp document/BrushManager.h
  document/Document.cpp document/Document.h
  document/Ids.h
  document/ProjectJson.cpp document/ProjectJson.h
  document/ImageStore.cpp document/ImageStore.h
  document/JsonWarnings.h
  document/LookJson.cpp document/LookJson.h
  document/LayerManager.cpp document/LayerManager.h
  document/ShapeLook.h
  document/StandardBrushes.cpp document/StandardBrushes.h
  document/ShapeManager.cpp document/ShapeManager.h
  document/StyleManager.cpp document/StyleManager.h
  edit/SelectionManager.cpp edit/SelectionManager.h
  edit/Tool.h
  edit/ToolManager.cpp edit/ToolManager.h
  edit/tools/EyedropperTool.cpp edit/tools/EyedropperTool.h
  edit/tools/LayerCheck.cpp edit/tools/LayerCheck.h
  edit/tools/PenStroke.cpp edit/tools/PenStroke.h
  edit/tools/PenTool.cpp edit/tools/PenTool.h
  edit/tools/ShapeTool.cpp edit/tools/ShapeTool.h
  edit/BrushActions.cpp edit/BrushActions.h
  edit/HitTest.cpp edit/HitTest.h
  edit/LookManager.cpp edit/LookManager.h
  edit/StyleActions.cpp edit/StyleActions.h
  edit/MacroCommand.h
  edit/SelectionCommands.cpp edit/SelectionCommands.h
  edit/tools/SelectTool.cpp edit/tools/SelectTool.h
  edit/LayerActions.cpp edit/LayerActions.h
  edit/LayerCommands.cpp edit/LayerCommands.h
  edit/ShapeCommands.cpp edit/ShapeCommands.h
  edit/StyleCommands.cpp edit/StyleCommands.h
  edit/UndoManager.cpp edit/UndoManager.h
  render/ArtCache.cpp render/ArtCache.h
  render/BrushStamper.cpp render/BrushStamper.h
  render/OverlayRenderer.cpp render/OverlayRenderer.h
  render/Renderer.cpp render/Renderer.h
  render/ShapeGeometry.cpp render/ShapeGeometry.h
  render/ViewTransform.cpp render/ViewTransform.h
  ui/BrushesPanel.cpp ui/BrushesPanel.h
  ui/BrushPreview.cpp ui/BrushPreview.h
  ui/CanvasView.cpp ui/CanvasView.h
  ui/ColorPopover.cpp ui/ColorPopover.h
  ui/ColorSwatch.cpp ui/ColorSwatch.h
  ui/CommandBox.cpp ui/CommandBox.h
  ui/DragTagger.h
  ui/LayerRowDelegate.cpp ui/LayerRowDelegate.h
  ui/LayersPanel.cpp ui/LayersPanel.h
  ui/MainWindow.cpp ui/MainWindow.h
  ui/NumberField.cpp ui/NumberField.h
  ui/PenOptions.cpp ui/PenOptions.h
  ui/EyedropperOptions.cpp ui/EyedropperOptions.h
  ui/LookCombos.cpp ui/LookCombos.h
  ui/LookEditor.cpp ui/LookEditor.h
  ui/StudyNoteBox.cpp ui/StudyNoteBox.h
  ui/StyleEditor.cpp ui/StyleEditor.h
  ui/StylesPanel.cpp ui/StylesPanel.h
  ui/ToolOptionsPanel.cpp ui/ToolOptionsPanel.h
  ui/ViewNavigator.cpp ui/ViewNavigator.h
)
