#include "edit/ToolManager.h"

#include "app/CommandRegistry.h"

namespace snapper {

ToolManager::ToolManager(QObject* parent) : QObject(parent) {}

ToolManager::~ToolManager() = default;

bool ToolManager::Add(std::unique_ptr<Tool> tool) {
  const bool is_full = static_cast<int>(tools_.size()) >= kMaxTools;
  const bool is_taken = tool == nullptr || IndexOf(tool->Info().id) >= 0;
  if (is_full || is_taken) {
    return false;
  }
  tools_.push_back(std::move(tool));
  if (active_ < 0) {
    active_ = 0;
  }
  return true;
}

bool ToolManager::Activate(const QString& id) {
  const int index = IndexOf(id);
  if (index < 0) {
    return false;
  }
  held_key_ = 0;
  SwitchTo(index);
  return true;
}

Tool* ToolManager::Active() const {
  return active_ < 0 ? nullptr : tools_[static_cast<size_t>(active_)].get();
}

QString ToolManager::ActiveId() const {
  const Tool* tool = Active();
  return tool == nullptr ? QString() : tool->Info().id;
}

void ToolManager::RegisterCommands(CommandRegistry& commands) {
  for (size_t i = 0; i < tools_.size(); ++i) {
    const ToolInfo info = tools_[i]->Info();
    const int index = static_cast<int>(i);
    const int key = info.key.isEmpty() ? 0 : info.key[0].key();
    commands.Add({
        .id = QStringLiteral("tool.%1").arg(info.id),
        .title = QStringLiteral("%1 Tool").arg(info.title),
        .shortcut = info.key,
        .run = [this, index, key] { ActivateFromKey(index, key); },
    });
  }
}

void ToolManager::Press(const ToolInput& input) {
  if (Tool* tool = Active()) {
    tool->Press(input);
    Q_EMIT OverlayChanged();
  }
}

void ToolManager::Drag(const ToolInput& input) {
  if (Tool* tool = Active()) {
    tool->Drag(input);
    Q_EMIT OverlayChanged();
  }
}

void ToolManager::Release(const ToolInput& input) {
  if (Tool* tool = Active()) {
    tool->Release(input);
    Q_EMIT OverlayChanged();
  }
  if (return_after_release_) {
    FinishQuickSwitch();
  }
}

void ToolManager::Hover(const ToolInput& input) {
  if (Tool* tool = Active()) {
    tool->Hover(input);
    Q_EMIT OverlayChanged();
  }
}

void ToolManager::DoubleClick(const ToolInput& input) {
  if (Tool* tool = Active()) {
    tool->DoubleClick(input);
    Q_EMIT OverlayChanged();
  }
}

void ToolManager::Leave() {
  if (Tool* tool = Active()) {
    tool->Leave();
    Q_EMIT OverlayChanged();
  }
}

void ToolManager::Cancel() {
  if (Tool* tool = Active()) {
    tool->Cancel();
    Q_EMIT OverlayChanged();
  }
}

void ToolManager::DrawOverlay(QPainter& painter,
                              const ViewTransform& view) const {
  if (const Tool* tool = Active()) {
    tool->DrawOverlay(painter, view);
  }
}

bool ToolManager::KeyPressed(int key, Qt::KeyboardModifiers modifiers) {
  Tool* tool = Active();
  const bool is_used = tool != nullptr && tool->KeyPress(key, modifiers);
  if (is_used) {
    Q_EMIT OverlayChanged();
  }
  return is_used;
}

// Letting go of a held tool key returns to the tool before. A tap leaves
// the new tool on. Mid-stroke, the return waits for the stroke to end.
void ToolManager::KeyReleased(int key) {
  const bool is_held_key = held_key_ != 0 && key == held_key_;
  if (!is_held_key) {
    return;
  }
  held_key_ = 0;
  const bool was_held = held_for_.elapsed() >= kHoldMs;
  if (!was_held) {
    return;
  }
  const Tool* tool = Active();
  const bool is_busy = tool != nullptr && tool->IsBusy();
  if (is_busy) {
    return_after_release_ = true;
  } else {
    FinishQuickSwitch();
  }
}

int ToolManager::IndexOf(const QString& id) const {
  for (size_t i = 0; i < tools_.size(); ++i) {
    if (tools_[i]->Info().id == id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

// Only a key that changes the tool starts a hold. Key repeats while held
// land here again and change nothing.
void ToolManager::ActivateFromKey(int index, int key) {
  if (index == active_) {
    return;
  }
  SwitchTo(index);
  held_key_ = key;
  held_for_.start();
}

void ToolManager::SwitchTo(int index) {
  if (index == active_) {
    return;
  }
  if (Tool* tool = Active()) {
    tool->Cancel();
  }
  previous_ = active_;
  active_ = index;
  return_after_release_ = false;
  const ToolInfo info = tools_[static_cast<size_t>(index)]->Info();
  Q_EMIT ActiveChanged(info.id);
  Q_EMIT Hint(info.hint);
  Q_EMIT OverlayChanged();
}

void ToolManager::FinishQuickSwitch() {
  return_after_release_ = false;
  if (previous_ >= 0) {
    SwitchTo(previous_);
  }
}

}  // namespace snapper
