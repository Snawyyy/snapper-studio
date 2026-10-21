#pragma once

#include <QString>

#include <functional>

#include "document/StyleManager.h"

namespace snapper {

class CommandRegistry;
class Document;
class LookManager;
class UndoManager;

// Everything you can do to styles as a whole: make, delete, rename and
// edit them. Each goes through undo. The Styles panel and the command list
// both call in here.
class StyleActions {
 public:
  StyleActions(Document& document, LookManager& looks, UndoManager& undo,
               std::function<void(const QString&)> hint);

  // Makes a style from the shown look, setting its fill, line and brush,
  // and gives it to the selected shapes, or to new shapes, as their first
  // style. Returns its id, or kNoId when the list is full.
  Id NewStyle();
  // Shapes using it keep their own look.
  void DeleteStyle(Id style);
  void RenameStyle(Id style, const QString& name);
  // Changes a style. Steps sharing a drag tag merge into one undo step.
  void EditStyle(Id style, const std::function<void(Style&)>& edit,
                 int drag_tag = 0);

  // What deleting the style would change, for its tooltip.
  QString DeleteEffect(Id style) const;

  // New Style.
  void RegisterCommands(CommandRegistry& commands);

 private:
  QString UnusedName() const;

  Document& document_;
  LookManager& looks_;
  UndoManager& undo_;
  std::function<void(const QString&)> hint_;
};

}  // namespace snapper
