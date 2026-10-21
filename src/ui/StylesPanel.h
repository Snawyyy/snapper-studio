#pragma once

#include <QWidget>

#include "app/Theme.h"
#include "document/Ids.h"

class QListWidget;
class QPushButton;

namespace snapper {

class ColorPopover;
class Document;
class LookManager;
class StyleActions;
class StyleEditor;

// The Styles panel, top to bottom: the look of the selected shapes (or of
// new shapes), the project's styles, and the editor for the picked style,
// always in that place. While the pointer is over the styles, the shapes
// the picked style changes are outlined on the canvas.
class StylesPanel : public QWidget {
  Q_OBJECT

 public:
  StylesPanel(Document& document, LookManager& looks, StyleActions& actions,
              ColorPopover& popover, const Theme& theme,
              QWidget* parent = nullptr);

 Q_SIGNALS:
  // The style whose shapes to outline, kNoId for none.
  void HighlightStyle(Id style);

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void Refill();
  void UpdateButtons();
  void UpdateHighlight();

  Document& document_;
  StyleActions& actions_;
  ColorPopover& popover_;
  const Theme& theme_;
  QWidget* styles_area_;
  QListWidget* list_;
  QPushButton* add_;
  QPushButton* delete_;
  StyleEditor* editor_;
  bool is_pointer_over_styles_ = false;
};

}  // namespace snapper
