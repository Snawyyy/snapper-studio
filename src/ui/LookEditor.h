#pragma once

#include <QWidget>

#include "document/StyleManager.h"
#include "ui/DragTagger.h"

class QCheckBox;
class QComboBox;
class QLabel;

namespace snapper {

class ColorPopover;
class ColorSwatch;
class Document;
class LookManager;
class NumberField;

// The top of the Styles panel: the look of the selected shapes, or of new
// shapes when nothing is selected. Fill, line, width and brush, and the
// two styles drawn over them. A part a style sets is greyed out and names
// the style.
class LookEditor : public QWidget {
  Q_OBJECT

 public:
  LookEditor(Document& document, LookManager& looks, ColorPopover& popover,
             QWidget* parent = nullptr);

 private:
  void Refresh();
  // Greys out a part a style sets, naming the style beside it.
  void ShowSource(LookPart part, QWidget* control, QLabel* note);

  Document& document_;
  LookManager& looks_;
  QLabel* title_;
  QCheckBox* fill_on_;
  ColorSwatch* fill_;
  QLabel* fill_note_;
  QCheckBox* line_on_;
  ColorSwatch* line_;
  QLabel* line_note_;
  NumberField* width_;
  QLabel* width_note_;
  QComboBox* brush_;
  QLabel* brush_note_;
  QComboBox* style1_;
  QComboBox* style2_;
  DragTagger width_drag_;
};

}  // namespace snapper
