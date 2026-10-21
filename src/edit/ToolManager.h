#pragma once

#include <QElapsedTimer>
#include <QObject>

#include <memory>
#include <vector>

#include "edit/Tool.h"

namespace snapper {

class CommandRegistry;

// Owns the canvas tools, which one is active, and the quick switch: hold a
// tool's key to use it, let go to return to the tool before.
class ToolManager : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMaxTools = 32;
  // Holding a tool key at least this long makes it a quick switch.
  static constexpr int kHoldMs = 300;

  explicit ToolManager(QObject* parent = nullptr);
  ~ToolManager() override;

  // The first tool added starts active. Returns false when full or when
  // the id is taken.
  bool Add(std::unique_ptr<Tool> tool);
  // Returns false when no tool has this id.
  bool Activate(const QString& id);

  Tool* Active() const;
  QString ActiveId() const;
  const std::vector<std::unique_ptr<Tool>>& All() const { return tools_; }

  // Adds a "<Tool> Tool" command per tool, on the tool's key.
  void RegisterCommands(CommandRegistry& commands);

  // From the canvas: pointer events go to the active tool.
  void Press(const ToolInput& input);
  void Drag(const ToolInput& input);
  void Release(const ToolInput& input);
  void Hover(const ToolInput& input);
  void DoubleClick(const ToolInput& input);
  void Leave();
  void Cancel();
  void DrawOverlay(QPainter& painter, const ViewTransform& view) const;
  // A key came up while the canvas had focus.
  void KeyReleased(int key);

 Q_SIGNALS:
  void ActiveChanged(const QString& id);
  // The overlay or preview changed and the canvas should redraw.
  void OverlayChanged();
  void Hint(const QString& text);

 private:
  int IndexOf(const QString& id) const;
  void ActivateFromKey(int index, int key);
  void SwitchTo(int index);
  void FinishQuickSwitch();

  std::vector<std::unique_ptr<Tool>> tools_;
  int active_ = -1;
  int previous_ = -1;
  // The tool key being held, while a quick switch may be under way.
  int held_key_ = 0;
  QElapsedTimer held_for_;
  bool return_after_release_ = false;
};

}  // namespace snapper
