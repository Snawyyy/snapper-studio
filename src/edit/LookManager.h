#pragma once

#include <QObject>
#include <QString>

#include <functional>
#include <vector>

#include "document/ShapeManager.h"
#include "document/StyleManager.h"

namespace snapper {

class CommandRegistry;
class Document;
class SelectionManager;
class UndoManager;

// Owns the look new shapes get, and changes the look of the selected
// shapes. A shape counts as selected when any of its points is. With
// nothing selected, look changes go to the look for new shapes.
class LookManager : public QObject {
  Q_OBJECT

 public:
  static constexpr float kMinLineWidth = 0.5F;
  static constexpr float kMaxLineWidth = 200.0F;
  // How much Thicker Line and Thinner Line change the width.
  static constexpr float kWidthStep = 1.25F;

  // `hint` says why a change can't show, like a width a style sets.
  LookManager(Document& document, SelectionManager& selection,
              UndoManager& undo, std::function<void(const QString&)> hint,
              QObject* parent = nullptr);

  // Shapes with a selected point, in drawing order.
  std::vector<Id> SelectedShapes() const;
  bool HasSelectedShapes() const { return !SelectedShapes().empty(); }

  // The look the Styles panel shows: the first selected shape's, or the
  // look for new shapes.
  ShapeLookState Shown() const;
  // The shown look with its styles applied, as it draws.
  ShapeLook ShownResolved() const;

  const ShapeLookState& NewShapes() const { return new_shapes_; }
  // The look new shapes draw with, their styles applied.
  ShapeLook NewShapesResolved() const;
  // Sets the look for new shapes, without its styles, as remembered from
  // last time.
  void RestoreNewShapes(const ShapeLook& look);

  // Changes the shown look: the selected shapes through undo, or the look
  // for new shapes. Steps sharing a drag tag merge into one undo step.
  void Change(const QString& text,
              const std::function<void(ShapeLookState&)>& edit,
              int drag_tag = 0);
  // Applies a whole look and styles, as the eyedropper picks them.
  void ChangeAll(const QString& text, const ShapeLook& look,
                 const StylePair& styles);

  // Thinner Line and Thicker Line, on [ and ].
  void RegisterCommands(CommandRegistry& commands);
  void StepWidth(bool thicker);

  // The name of the style that sets this part of the shown look, or empty
  // when the shape's own look does.
  QString StyleSetting(LookPart part) const;

 Q_SIGNALS:
  // What Shown() returns may have changed.
  void ShownChanged();
  // The look for new shapes changed, so it can be remembered.
  void NewShapesChanged();

 private:
  // Keeps the new shapes' styles pointing at styles that exist. A new or
  // loaded project starts them on its first style.
  void KeepStylesValid(bool is_new_project);

  Document& document_;
  SelectionManager& selection_;
  UndoManager& undo_;
  std::function<void(const QString&)> hint_;
  ShapeLookState new_shapes_;
};

}  // namespace snapper
