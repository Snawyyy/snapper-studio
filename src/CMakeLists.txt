target_sources(snapper_core PRIVATE
  app/CommandRegistry.cpp app/CommandRegistry.h
  app/SettingsManager.cpp app/SettingsManager.h
  app/Startup.cpp app/Startup.h
  app/Studio.cpp app/Studio.h
  app/Theme.h
  app/ThemeManager.cpp app/ThemeManager.h
  document/Document.cpp document/Document.h
  document/Ids.h
  document/LayerManager.cpp document/LayerManager.h
  document/ShapeManager.cpp document/ShapeManager.h
  edit/LayerCommands.cpp edit/LayerCommands.h
  edit/ShapeCommands.cpp edit/ShapeCommands.h
  edit/UndoManager.cpp edit/UndoManager.h
  render/Renderer.cpp render/Renderer.h
  render/ShapeGeometry.cpp render/ShapeGeometry.h
  render/ViewTransform.cpp render/ViewTransform.h
  ui/CanvasView.cpp ui/CanvasView.h
  ui/CommandBox.cpp ui/CommandBox.h
  ui/MainWindow.cpp ui/MainWindow.h
)
