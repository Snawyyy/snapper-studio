#include "edit/StyleCommands.h"

#include "document/Document.h"

namespace snapper {

std::vector<ShapeLookState> LookStatesOf(const ShapeManager& shapes,
                                         const std::vector<Id>& ids) {
  std::vector<ShapeLookState> states;
  for (const Id id : ids) {
    const Shape* shape = shapes.Find(id);
    if (shape != nullptr) {
      states.push_back(
          {.shape = id, .look = shape->look, .styles = shape->styles});
    }
  }
  return states;
}

SetStyleCommand::SetStyleCommand(Document& document, const Style& before,
                                 const Style& after, int drag_tag)
    : ReplaceValueCommand(document,
                          QStringLiteral("Edit Style %1").arg(after.name),
                          before, after, drag_tag) {}

void SetStyleCommand::Apply(const Style& style) { Doc().Styles().Set(style); }

SetBrushCommand::SetBrushCommand(Document& document, const Brush& before,
                                 const Brush& after, int drag_tag)
    : ReplaceValueCommand(document,
                          QStringLiteral("Edit Brush %1").arg(after.name),
                          before, after, drag_tag) {}

void SetBrushCommand::Apply(const Brush& brush) { Doc().Brushes().Set(brush); }

SetShapeLooksCommand::SetShapeLooksCommand(Document& document,
                                           const QString& text,
                                           std::vector<ShapeLookState> before,
                                           std::vector<ShapeLookState> after,
                                           int drag_tag)
    : ReplaceValueCommand(document, text, std::move(before), std::move(after),
                          drag_tag) {}

void SetShapeLooksCommand::Apply(const std::vector<ShapeLookState>& looks) {
  Doc().Shapes().SetLooks(looks);
}

AddStyleCommand::AddStyleCommand(Document& document, const Style& style,
                                 int index)
    : QUndoCommand(QStringLiteral("New Style")),
      document_(document),
      style_(style),
      index_(index) {}

void AddStyleCommand::redo() { document_.Styles().Insert(style_, index_); }
void AddStyleCommand::undo() { document_.Styles().Take(style_.id); }

RemoveStyleCommand::RemoveStyleCommand(Document& document, Id style)
    : QUndoCommand(QStringLiteral("Delete Style")), document_(document) {
  const Style* found = document.Styles().Find(style);
  if (found != nullptr) {
    style_ = {.style = *found, .index = document.Styles().IndexOf(style)};
    setText(QStringLiteral("Delete Style %1").arg(found->name));
  }
  users_before_ =
      LookStatesOf(document.Shapes(), document.Shapes().ShapesWithStyle(style));
  users_after_ = users_before_;
  for (ShapeLookState& state : users_after_) {
    for (Id& slot : state.styles) {
      if (slot == style) {
        slot = kNoId;
      }
    }
  }
}

void RemoveStyleCommand::redo() {
  document_.Shapes().SetLooks(users_after_);
  document_.Styles().Take(style_.style.id);
}

void RemoveStyleCommand::undo() {
  document_.Styles().Insert(style_.style, style_.index);
  document_.Shapes().SetLooks(users_before_);
}

AddBrushCommand::AddBrushCommand(Document& document, const Brush& brush,
                                 int index)
    : QUndoCommand(QStringLiteral("Import Brush")),
      document_(document),
      brush_(brush),
      index_(index) {}

void AddBrushCommand::redo() { document_.Brushes().Insert(brush_, index_); }
void AddBrushCommand::undo() { document_.Brushes().Take(brush_.id); }

RemoveBrushCommand::RemoveBrushCommand(Document& document, Id brush)
    : QUndoCommand(QStringLiteral("Delete Brush")), document_(document) {
  const Brush* found = document.Brushes().Find(brush);
  if (found != nullptr) {
    brush_ = {.brush = *found, .index = document.Brushes().IndexOf(brush)};
    setText(QStringLiteral("Delete Brush %1").arg(found->name));
  }
  shapes_before_ =
      LookStatesOf(document.Shapes(), document.Shapes().ShapesWithBrush(brush));
  shapes_after_ = shapes_before_;
  for (ShapeLookState& state : shapes_after_) {
    state.look.brush = kNoId;
  }
  for (const Style& style : document.Styles().All()) {
    if (style.look.brush == brush) {
      styles_before_.push_back(style);
      Style plain = style;
      plain.look.brush = kNoId;
      styles_after_.push_back(plain);
    }
  }
}

void RemoveBrushCommand::redo() {
  document_.Shapes().SetLooks(shapes_after_);
  for (const Style& style : styles_after_) {
    document_.Styles().Set(style);
  }
  document_.Brushes().Take(brush_.brush.id);
}

void RemoveBrushCommand::undo() {
  document_.Brushes().Insert(brush_.brush, brush_.index);
  for (const Style& style : styles_before_) {
    document_.Styles().Set(style);
  }
  document_.Shapes().SetLooks(shapes_before_);
}

}  // namespace snapper
