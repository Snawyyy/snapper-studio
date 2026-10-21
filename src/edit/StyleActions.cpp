#include "edit/StyleActions.h"

#include <algorithm>

#include "app/CommandRegistry.h"
#include "document/Document.h"
#include "edit/LookManager.h"
#include "edit/MacroCommand.h"
#include "edit/StyleCommands.h"
#include "edit/UndoManager.h"

namespace snapper {

StyleActions::StyleActions(Document& document, LookManager& looks,
                           UndoManager& undo,
                           std::function<void(const QString&)> hint)
    : document_(document), looks_(looks), undo_(undo), hint_(std::move(hint)) {}

Id StyleActions::NewStyle() {
  if (document_.Styles().Count() >= StyleManager::kMaxStyles) {
    hint_(QStringLiteral("New Style: a project holds at most %1 styles")
              .arg(StyleManager::kMaxStyles));
    return kNoId;
  }
  Style style{.id = document_.NewId(),
              .name = UnusedName(),
              .look = looks_.ShownResolved()};
  style.parts.fill = true;
  style.parts.line = true;
  style.parts.brush = true;
  // One step: the new style and its first use.
  auto macro = std::make_unique<MacroCommand>(
      QStringLiteral("New Style %1").arg(style.name));
  macro->Add(std::make_unique<AddStyleCommand>(document_, style,
                                               document_.Styles().Count()));
  const std::vector<ShapeLookState> before =
      LookStatesOf(document_.Shapes(), looks_.SelectedShapes());
  if (!before.empty()) {
    std::vector<ShapeLookState> after = before;
    for (ShapeLookState& state : after) {
      state.styles[0] = style.id;
    }
    macro->Add(std::make_unique<SetShapeLooksCommand>(document_, QString(),
                                                      before, after, 0));
  }
  undo_.Push(std::move(macro));
  if (before.empty()) {
    looks_.Change(QString(),
                  [&](ShapeLookState& state) { state.styles[0] = style.id; });
  }
  return style.id;
}

void StyleActions::DeleteStyle(Id style) {
  if (document_.Styles().Find(style) != nullptr) {
    undo_.Push(std::make_unique<RemoveStyleCommand>(document_, style));
  }
}

void StyleActions::RenameStyle(Id style, const QString& name) {
  const Style* found = document_.Styles().Find(style);
  const QString trimmed = name.trimmed();
  const bool is_change =
      found != nullptr && !trimmed.isEmpty() && found->name != trimmed;
  if (!is_change) {
    return;
  }
  Style renamed = *found;
  renamed.name = trimmed;
  auto command =
      std::make_unique<SetStyleCommand>(document_, *found, renamed, 0);
  command->setText(QStringLiteral("Rename Style %1").arg(trimmed));
  undo_.Push(std::move(command));
}

void StyleActions::EditStyle(Id style, const std::function<void(Style&)>& edit,
                             int drag_tag) {
  const Style* found = document_.Styles().Find(style);
  if (found == nullptr) {
    return;
  }
  Style after = *found;
  edit(after);
  after.id = style;
  if (after == *found) {
    return;
  }
  undo_.Push(
      std::make_unique<SetStyleCommand>(document_, *found, after, drag_tag));
}

QString StyleActions::DeleteEffect(Id style) const {
  const int users =
      static_cast<int>(document_.Shapes().ShapesWithStyle(style).size());
  if (users == 0) {
    return QStringLiteral("Delete the style. No shape uses it.");
  }
  return QStringLiteral("Delete the style. %1 %2 it and keep their own look.")
      .arg(users)
      .arg(users == 1 ? QStringLiteral("shape uses")
                      : QStringLiteral("shapes use"));
}

void StyleActions::RegisterCommands(CommandRegistry& commands) {
  commands.Add({
      .id = QStringLiteral("style.new"),
      .title = QStringLiteral("New Style"),
      .shortcut = {},
      .run = [this] { NewStyle(); },
  });
}

QString StyleActions::UnusedName() const {
  // Of count + 1 numbers in a row, at least one is free.
  const std::vector<Style>& styles = document_.Styles().All();
  const int count = static_cast<int>(styles.size());
  for (int n = count + 1; n <= 2 * count + 1; ++n) {
    const QString name = QStringLiteral("Style %1").arg(n);
    const bool is_taken = std::ranges::any_of(
        styles, [&](const Style& style) { return style.name == name; });
    if (!is_taken) {
      return name;
    }
  }
  return QStringLiteral("Style");
}

}  // namespace snapper
