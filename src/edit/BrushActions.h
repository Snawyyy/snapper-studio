#pragma once

#include <QString>

#include <functional>

#include "document/BrushManager.h"

namespace snapper {

class Document;
class LookManager;
class UndoManager;

// Everything you can do to brushes as a whole: import, delete, rename and
// edit them, and draw the shown look with one. Each goes through undo.
class BrushActions {
 public:
  // A brush picture is a single dab, so it never needs to be big.
  static constexpr int kMaxPictureSide = 1024;

  BrushActions(Document& document, LookManager& looks, UndoManager& undo,
               std::function<void(const QString&)> hint);

  // Adds a picture file as a brush, named after the file. Its alpha is
  // the paint, or its dark parts when it has no transparency. Returns its
  // id, or kNoId with the reason.
  Id ImportBrush(const QString& path, QString* why_not);
  // Shapes and styles using it draw plain lines.
  void DeleteBrush(Id brush);
  void RenameBrush(Id brush, const QString& name);
  // Changes a brush. Steps sharing a drag tag merge into one undo step.
  void EditBrush(Id brush, const std::function<void(Brush&)>& edit,
                 int drag_tag = 0);
  // Draws the selected shapes, or new shapes, with the brush.
  void UseBrush(Id brush);

  // What deleting the brush would change, for its tooltip.
  QString DeleteEffect(Id brush) const;

 private:
  Document& document_;
  LookManager& looks_;
  UndoManager& undo_;
  std::function<void(const QString&)> hint_;
};

}  // namespace snapper
