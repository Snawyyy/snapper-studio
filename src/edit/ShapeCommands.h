#pragma once

#include <QUndoCommand>

#include <memory>
#include <vector>

#include "document/Ids.h"
#include "document/ShapeManager.h"

namespace snapper {

class Document;

// Takes every shape in `out` out of the list, then inserts every shape in
// `in` at its index. Indexes are where each shape sits in the list before
// (out) and after (in), so the same call undoes itself with the two swapped.
void SwapShapes(ShapeManager& shapes, const std::vector<RemovedShape>& out,
                const std::vector<RemovedShape>& in);

// Any change that adds, removes or rewrites whole shapes.
class ReplaceShapesCommand : public QUndoCommand {
 public:
  ReplaceShapesCommand(Document& document, const QString& text,
                       std::vector<RemovedShape> before,
                       std::vector<RemovedShape> after);

  void redo() override;
  void undo() override;

 private:
  Document& document_;
  std::vector<RemovedShape> before_;
  std::vector<RemovedShape> after_;
};

// Moving points. Steps of one drag merge into one undo step.
class MovePointsCommand : public QUndoCommand {
 public:
  static constexpr int kMergeId = 1;

  // Each drag gets its own tag, so two drags in a row stay two steps.
  static int NewDragTag();

  MovePointsCommand(Document& document, std::vector<PointPosition> from,
                    std::vector<PointPosition> to, int drag_tag);

  void redo() override;
  void undo() override;
  int id() const override { return kMergeId; }
  bool mergeWith(const QUndoCommand* other) override;

 private:
  Document& document_;
  std::vector<PointPosition> from_;
  std::vector<PointPosition> to_;
  int drag_tag_;
};

// Adds a shape on top of everything else.
std::unique_ptr<QUndoCommand> MakeAddShape(Document& document,
                                           const Shape& shape,
                                           const QString& text);

// Removes points from their shapes. A shape left with fewer than two points
// goes too. Null when none of the points exist.
std::unique_ptr<QUndoCommand> MakeRemovePoints(Document& document,
                                               const std::vector<Id>& points);

}  // namespace snapper
