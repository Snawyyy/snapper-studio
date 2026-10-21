#include "edit/ShapeCommands.h"

#include <QSet>

#include <algorithm>

#include "document/Document.h"

namespace snapper {

namespace {

bool ByIndex(const RemovedShape& a, const RemovedShape& b) {
  return a.index < b.index;
}

}  // namespace

void SwapShapes(ShapeManager& shapes, const std::vector<RemovedShape>& out,
                const std::vector<RemovedShape>& in) {
  // Back to front, so taking one never shifts the next.
  for (auto it = out.rbegin(); it != out.rend(); ++it) {
    shapes.Take(it->shape.id);
  }
  // Front to back, so each lands at its final index.
  for (const RemovedShape& item : in) {
    shapes.Insert(item.shape, item.index);
  }
}

ReplaceShapesCommand::ReplaceShapesCommand(Document& document,
                                           const QString& text,
                                           std::vector<RemovedShape> before,
                                           std::vector<RemovedShape> after)
    : QUndoCommand(text),
      document_(document),
      before_(std::move(before)),
      after_(std::move(after)) {
  std::ranges::sort(before_, ByIndex);
  std::ranges::sort(after_, ByIndex);
}

void ReplaceShapesCommand::redo() {
  SwapShapes(document_.Shapes(), before_, after_);
}

void ReplaceShapesCommand::undo() {
  SwapShapes(document_.Shapes(), after_, before_);
}

int MovePointsCommand::NewDragTag() {
  static int next_tag = 0;
  return ++next_tag;
}

MovePointsCommand::MovePointsCommand(Document& document,
                                     std::vector<PointPosition> from,
                                     std::vector<PointPosition> to,
                                     int drag_tag)
    : QUndoCommand(QStringLiteral("Move Points")),
      document_(document),
      from_(std::move(from)),
      to_(std::move(to)),
      drag_tag_(drag_tag) {}

void MovePointsCommand::redo() { document_.Shapes().SetPositions(to_); }
void MovePointsCommand::undo() { document_.Shapes().SetPositions(from_); }

// Matching ids make this cast safe: only this class reports kMergeId.
bool MovePointsCommand::mergeWith(const QUndoCommand* other) {
  const bool is_same_kind = other->id() == kMergeId;
  if (!is_same_kind) {
    return false;
  }
  const auto* move = static_cast<const MovePointsCommand*>(other);
  if (move->drag_tag_ != drag_tag_) {
    return false;
  }
  to_ = move->to_;
  return true;
}

std::unique_ptr<QUndoCommand> MakeAddShape(Document& document,
                                           const Shape& shape,
                                           const QString& text) {
  std::vector<RemovedShape> after = {
      {.shape = shape, .index = document.Shapes().Count()}};
  return std::make_unique<ReplaceShapesCommand>(
      document, text, std::vector<RemovedShape>{}, std::move(after));
}

std::unique_ptr<QUndoCommand> MakeRemovePoints(Document& document,
                                               const std::vector<Id>& points) {
  const ShapeManager& shapes = document.Shapes();
  QSet<Id> doomed;
  QSet<Id> touched_shapes;
  for (const Id point : points) {
    const Id shape = shapes.ShapeOfPoint(point);
    if (shape != kNoId) {
      doomed.insert(point);
      touched_shapes.insert(shape);
    }
  }
  if (touched_shapes.isEmpty()) {
    return nullptr;
  }

  std::vector<RemovedShape> before;
  for (const Id id : touched_shapes) {
    before.push_back({.shape = *shapes.Find(id), .index = shapes.IndexOf(id)});
  }
  std::ranges::sort(before, ByIndex);

  // A shape that loses everything shifts the ones after it down by one.
  std::vector<RemovedShape> after;
  int shapes_gone = 0;
  for (const RemovedShape& item : before) {
    Shape kept = item.shape;
    std::erase_if(kept.points, [&](const Point& point) {
      return doomed.contains(point.id);
    });
    const bool is_still_a_shape = kept.points.size() >= 2;
    if (is_still_a_shape) {
      kept.closed = kept.closed && kept.points.size() >= 3;
      after.push_back({.shape = kept, .index = item.index - shapes_gone});
    } else {
      ++shapes_gone;
    }
  }
  return std::make_unique<ReplaceShapesCommand>(
      document, QStringLiteral("Delete Points"), std::move(before),
      std::move(after));
}

}  // namespace snapper
