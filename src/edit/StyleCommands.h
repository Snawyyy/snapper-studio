#pragma once

#include <QUndoCommand>

#include <vector>

#include "document/BrushManager.h"
#include "document/ShapeManager.h"
#include "document/StyleManager.h"

namespace snapper {

class Document;

// Swaps a value for another, and back on undo. Steps of one drag share a
// drag tag (MovePointsCommand::NewDragTag) and merge into one undo step,
// so dragging a value is one step. Tag 0 never merges.
template <typename Value, int kMergeIdValue>
class ReplaceValueCommand : public QUndoCommand {
 public:
  ReplaceValueCommand(Document& document, const QString& text, Value before,
                      Value after, int drag_tag)
      : QUndoCommand(text),
        document_(document),
        before_(std::move(before)),
        after_(std::move(after)),
        drag_tag_(drag_tag) {}

  void redo() override { Apply(after_); }
  void undo() override { Apply(before_); }
  int id() const override { return kMergeIdValue; }

  // Matching ids make this cast safe: only this class reports its id.
  bool mergeWith(const QUndoCommand* other) override {
    const bool is_same_kind = other->id() == kMergeIdValue;
    if (!is_same_kind) {
      return false;
    }
    const auto* next = static_cast<const ReplaceValueCommand*>(other);
    const bool is_same_drag = drag_tag_ != 0 && next->drag_tag_ == drag_tag_;
    if (!is_same_drag) {
      return false;
    }
    after_ = next->after_;
    return true;
  }

 protected:
  virtual void Apply(const Value& value) = 0;
  Document& Doc() { return document_; }

 private:
  Document& document_;
  Value before_;
  Value after_;
  int drag_tag_;
};

class SetStyleCommand final : public ReplaceValueCommand<Style, 2> {
 public:
  SetStyleCommand(Document& document, const Style& before, const Style& after,
                  int drag_tag);

 private:
  void Apply(const Style& style) override;
};

class SetBrushCommand final : public ReplaceValueCommand<Brush, 3> {
 public:
  SetBrushCommand(Document& document, const Brush& before, const Brush& after,
                  int drag_tag);

 private:
  void Apply(const Brush& brush) override;
};

// Sets shapes' own looks and styles.
class SetShapeLooksCommand final
    : public ReplaceValueCommand<std::vector<ShapeLookState>, 4> {
 public:
  SetShapeLooksCommand(Document& document, const QString& text,
                       std::vector<ShapeLookState> before,
                       std::vector<ShapeLookState> after, int drag_tag);

 private:
  void Apply(const std::vector<ShapeLookState>& looks) override;
};

class AddStyleCommand : public QUndoCommand {
 public:
  AddStyleCommand(Document& document, const Style& style, int index);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  Style style_;
  int index_;
};

// Removes a style. Shapes using it keep their own look and their other
// style.
class RemoveStyleCommand : public QUndoCommand {
 public:
  RemoveStyleCommand(Document& document, Id style);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  RemovedStyle style_;
  std::vector<ShapeLookState> users_before_;
  std::vector<ShapeLookState> users_after_;
};

class AddBrushCommand : public QUndoCommand {
 public:
  AddBrushCommand(Document& document, const Brush& brush, int index);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  Brush brush_;
  int index_;
};

// Removes a brush. Shapes and styles using it draw plain lines.
class RemoveBrushCommand : public QUndoCommand {
 public:
  RemoveBrushCommand(Document& document, Id brush);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  RemovedBrush brush_;
  std::vector<ShapeLookState> shapes_before_;
  std::vector<ShapeLookState> shapes_after_;
  std::vector<Style> styles_before_;
  std::vector<Style> styles_after_;
};

// The look state of each shape, for building look commands. Unknown ids
// are skipped.
std::vector<ShapeLookState> LookStatesOf(const ShapeManager& shapes,
                                         const std::vector<Id>& ids);

}  // namespace snapper
