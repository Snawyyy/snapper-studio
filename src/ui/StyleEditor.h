#pragma once

#include <QWidget>

#include "document/StyleManager.h"
#include "ui/DragTagger.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;

namespace snapper {

class ColorPopover;
class ColorSwatch;
class Document;
class NumberField;
class StyleActions;

// Edits one style: its name and each part it sets. Ticking a part makes
// the style set it, and changing a value ticks it too. Every change shows
// on the canvas at once.
class StyleEditor : public QWidget {
  Q_OBJECT

 public:
  StyleEditor(Document& document, StyleActions& actions, ColorPopover& popover,
              QWidget* parent = nullptr);

  void SetStyle(Id style);
  Id CurrentStyle() const { return style_; }
  // True while the color picker is open for one of its colors.
  bool IsPicking() const;

 private:
  void Refresh();
  void Edit(const std::function<void(Style&)>& edit, int drag_tag = 0);

  Document& document_;
  StyleActions& actions_;
  ColorPopover& popover_;
  Id style_ = kNoId;
  QLabel* users_;
  QLineEdit* name_;
  QCheckBox* sets_fill_;
  ColorSwatch* fill_;
  QCheckBox* no_fill_;
  QCheckBox* sets_line_;
  ColorSwatch* line_;
  QCheckBox* no_line_;
  QCheckBox* sets_width_;
  NumberField* width_;
  QCheckBox* sets_brush_;
  QComboBox* brush_;
  DragTagger width_drag_;
};

}  // namespace snapper
