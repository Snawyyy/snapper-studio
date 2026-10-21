#pragma once

#include <QElapsedTimer>

#include "edit/ShapeCommands.h"

namespace snapper {

// Hands out one drag tag for a run of quick changes to one field, so
// scrubbing or scrolling a number makes one undo step.
class DragTagger {
 public:
  static constexpr qint64 kPauseMs = 700;

  int Tag() {
    const bool is_new_run = !since_.isValid() || since_.elapsed() > kPauseMs;
    if (is_new_run) {
      tag_ = MovePointsCommand::NewDragTag();
    }
    since_.start();
    return tag_;
  }

 private:
  QElapsedTimer since_;
  int tag_ = 0;
};

}  // namespace snapper
