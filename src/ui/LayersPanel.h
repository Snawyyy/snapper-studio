#pragma once

#include <QWidget>

#include "app/Theme.h"
#include "document/Ids.h"

class QListWidget;
class QPushButton;

namespace snapper {

class CommandRegistry;
class Document;
class LayerActions;
class SelectionManager;

// Lists the layers, top first, with the layers inside a bone layer below
// it, indented. Click a row to draw on that layer, click the eye or
// padlock to show, hide, lock or unlock it. Double click, F2, or a click on
// the selected name renames it. Drag a row to restack it, or onto a bone
// layer to put it inside.
class LayersPanel : public QWidget {
  Q_OBJECT

 public:
  LayersPanel(Document& document, SelectionManager& selection,
              LayerActions& actions, const Theme& theme,
              QWidget* parent = nullptr);

  // Adds Rename Layer.
  void RegisterCommands(CommandRegistry& commands);
  void RenameActive();
  // Asks for a picture file and adds it as an image layer.
  void ImportPicture();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void Refill();
  void UpdateButtons();
  // Handles a press on a row's eye or padlock. True when it hit one.
  bool HandleIconPress(const QPoint& pos);
  // Mouse events on the list for dragging rows. True when used.
  bool HandleDrag(const QEvent* event);
  // Marks where a row dragged to `pos` would land.
  void ShowDropAt(const QPoint& pos);
  void ClearDrop();

  Document& document_;
  SelectionManager& selection_;
  LayerActions& actions_;
  QListWidget* list_;
  QPushButton* add_;
  QPushButton* add_bones_;
  QPushButton* delete_;
  // A row pressed, which may become a drag.
  Id pressed_ = kNoId;
  QPoint press_pos_;
  bool is_dragging_ = false;
  // Where the drag would land now.
  Id drop_target_ = kNoId;
  int drop_spot_ = 0;
};

}  // namespace snapper
