#pragma once

#include <QWidget>

#include "app/Theme.h"

class QListWidget;
class QPushButton;

namespace snapper {

class CommandRegistry;
class Document;
class LayerActions;
class SelectionManager;

// Lists the layers, top first. Click a row to draw on that layer, click
// the eye or padlock to show, hide, lock or unlock it. Double click, F2,
// or a click on the selected name renames it.
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

  Document& document_;
  SelectionManager& selection_;
  LayerActions& actions_;
  QListWidget* list_;
  QPushButton* add_;
  QPushButton* delete_;
};

}  // namespace snapper
