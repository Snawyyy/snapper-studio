#include "edit/LookManager.h"

#include <algorithm>

#include "app/CommandRegistry.h"
#include "document/Document.h"
#include "edit/SelectionManager.h"
#include "edit/StyleCommands.h"
#include "edit/UndoManager.h"

namespace snapper {

LookManager::LookManager(Document& document, SelectionManager& selection,
                         UndoManager& undo,
                         std::function<void(const QString&)> hint,
                         QObject* parent)
    : QObject(parent),
      document_(document),
      selection_(selection),
      undo_(undo),
      hint_(std::move(hint)) {
  const auto shown_changed = [this] { Q_EMIT ShownChanged(); };
  connect(&selection_, &SelectionManager::PointsChanged, this, shown_changed);
  connect(&selection_, &SelectionManager::ActiveLayerChanged, this,
          shown_changed);
  connect(&document_.Shapes(), &ShapeManager::LayerContentChanged, this,
          shown_changed);
  connect(&document_.Styles(), &StyleManager::StyleChanged, this,
          shown_changed);
  connect(&document_.Styles(), &StyleManager::ListChanged, this,
          [this] { KeepStylesValid(false); });
  connect(&document_.Brushes(), &BrushManager::ListChanged, this,
          [this] { KeepStylesValid(false); });
  connect(&document_, &Document::Reset, this,
          [this] { KeepStylesValid(true); });
  KeepStylesValid(true);
}

std::vector<Id> LookManager::SelectedShapes() const {
  std::vector<Id> ids;
  const QSet<Id>& points = selection_.Points();
  if (points.isEmpty()) {
    return ids;
  }
  for (const Shape& shape : document_.Shapes().All()) {
    const bool has_selected_point = std::ranges::any_of(
        shape.points,
        [&](const Point& point) { return points.contains(point.id); });
    if (has_selected_point) {
      ids.push_back(shape.id);
    }
  }
  return ids;
}

ShapeLookState LookManager::Shown() const {
  const std::vector<Id> selected = SelectedShapes();
  if (selected.empty()) {
    return new_shapes_;
  }
  return LookStatesOf(document_.Shapes(), {selected.front()}).front();
}

ShapeLook LookManager::ShownResolved() const {
  const ShapeLookState shown = Shown();
  return document_.Styles().Resolve(shown.look, shown.styles);
}

ShapeLook LookManager::NewShapesResolved() const {
  return document_.Styles().Resolve(new_shapes_.look, new_shapes_.styles);
}

void LookManager::RestoreNewShapes(const ShapeLook& look) {
  new_shapes_.look = look;
  new_shapes_.look.line_width =
      std::clamp(look.line_width, kMinLineWidth, kMaxLineWidth);
  // Brush ids belong to one project, so a remembered one means nothing.
  new_shapes_.look.brush = kNoId;
  Q_EMIT ShownChanged();
}

void LookManager::Change(const QString& text,
                         const std::function<void(ShapeLookState&)>& edit,
                         int drag_tag) {
  const std::vector<Id> selected = SelectedShapes();
  if (selected.empty()) {
    edit(new_shapes_);
    Q_EMIT NewShapesChanged();
    Q_EMIT ShownChanged();
    return;
  }
  const std::vector<ShapeLookState> before =
      LookStatesOf(document_.Shapes(), selected);
  std::vector<ShapeLookState> after = before;
  for (ShapeLookState& state : after) {
    edit(state);
  }
  if (after == before) {
    return;
  }
  undo_.Push(std::make_unique<SetShapeLooksCommand>(document_, text, before,
                                                    after, drag_tag));
}

void LookManager::ChangeAll(const QString& text, const ShapeLook& look,
                            const StylePair& styles) {
  Change(text, [&](ShapeLookState& state) {
    state.look = look;
    state.styles = styles;
  });
}

QString LookManager::StyleSetting(LookPart part) const {
  const ShapeLookState shown = Shown();
  // The second style wins, so it is asked first.
  for (auto it = shown.styles.rbegin(); it != shown.styles.rend(); ++it) {
    const Style* style = document_.Styles().Find(*it);
    const bool sets_part = style != nullptr && style->parts.Sets(part);
    if (sets_part) {
      return style->name;
    }
  }
  return {};
}

void LookManager::StepWidth(bool thicker) {
  const QString style = StyleSetting(LookPart::kWidth);
  if (!style.isEmpty()) {
    hint_(QStringLiteral("Line width comes from the style %1: edit the style "
                         "to change it")
              .arg(style));
    return;
  }
  const float factor = thicker ? kWidthStep : 1.0F / kWidthStep;
  Change(
      thicker ? QStringLiteral("Thicker Line") : QStringLiteral("Thinner Line"),
      [factor](ShapeLookState& state) {
        state.look.line_width = std::clamp(state.look.line_width * factor,
                                           kMinLineWidth, kMaxLineWidth);
      });
  hint_(QStringLiteral("Line width %1 px")
            .arg(static_cast<double>(Shown().look.line_width), 0, 'f', 1));
}

void LookManager::RegisterCommands(CommandRegistry& commands) {
  commands.Add({
      .id = QStringLiteral("look.thinner"),
      .title = QStringLiteral("Thinner Line"),
      .shortcut = QKeySequence(Qt::Key_BracketLeft),
      .run = [this] { StepWidth(false); },
  });
  commands.Add({
      .id = QStringLiteral("look.thicker"),
      .title = QStringLiteral("Thicker Line"),
      .shortcut = QKeySequence(Qt::Key_BracketRight),
      .run = [this] { StepWidth(true); },
  });
}

void LookManager::KeepStylesValid(bool is_new_project) {
  const StyleManager& styles = document_.Styles();
  if (is_new_project) {
    // Like Moho, new shapes start with no styles, so the look editor's
    // colors and brush go straight onto them.
    new_shapes_.styles = {kNoId, kNoId};
    new_shapes_.look.brush = kNoId;
  }
  for (Id& style : new_shapes_.styles) {
    if (styles.Find(style) == nullptr) {
      style = kNoId;
    }
  }
  if (document_.Brushes().Find(new_shapes_.look.brush) == nullptr) {
    new_shapes_.look.brush = kNoId;
  }
  Q_EMIT ShownChanged();
}

}  // namespace snapper
