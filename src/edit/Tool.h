#pragma once

#include <QKeySequence>
#include <QPointF>
#include <QString>

#include <functional>

class QPainter;

namespace snapper {

class Document;
class SelectionManager;
class UndoManager;
class ViewTransform;

// One pointer event on the canvas, from a pen or a mouse.
struct ToolInput {
  QPointF canvas;
  QPointF screen;
  // 0 to 1. A mouse always presses at 1.
  float pressure = 1.0F;
  Qt::KeyboardModifiers modifiers;
  // Canvas pixels per screen pixel, so tools can size things on screen.
  double canvas_per_screen = 1.0;
};

// What every tool works with.
struct ToolContext {
  Document& document;
  UndoManager& undo;
  SelectionManager& selection;
  // Shows a line in the status bar naming the next step or what went
  // wrong. An empty text clears it.
  std::function<void(const QString&)> hint;
};

struct ToolInfo {
  QString id;     // stable: "pen"
  QString title;  // shown: "Pen"
  QKeySequence key;
  QString hint;  // shown in the status bar while the tool is active
};

// A canvas tool. The tool manager hands it pointer events while it is
// active. Every change it makes goes through undo.
class Tool {
 public:
  explicit Tool(ToolContext context) : context_(std::move(context)) {}
  virtual ~Tool() = default;
  Tool(const Tool&) = delete;
  Tool& operator=(const Tool&) = delete;

  virtual ToolInfo Info() const = 0;

  virtual void Press(const ToolInput& /*input*/) {}
  virtual void Drag(const ToolInput& /*input*/) {}
  virtual void Release(const ToolInput& /*input*/) {}
  virtual void Hover(const ToolInput& /*input*/) {}
  virtual void DoubleClick(const ToolInput& /*input*/) {}
  // The pointer left the canvas.
  virtual void Leave() {}
  // Drops whatever is in progress, for Escape or a tool switch mid-drag.
  virtual void Cancel() {}
  // Draws previews and handles over the artwork, in screen coordinates.
  virtual void DrawOverlay(QPainter& /*painter*/,
                           const ViewTransform& /*view*/) const {}
  virtual bool IsBusy() const { return false; }

 protected:
  ToolContext& Context() { return context_; }
  const ToolContext& Context() const { return context_; }

 private:
  ToolContext context_;
};

}  // namespace snapper
