#include "ui/LayersPanel.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

#include "app/CommandRegistry.h"
#include "document/Document.h"
#include "edit/LayerActions.h"
#include "edit/SelectionManager.h"
#include "ui/LayerRowDelegate.h"

namespace snapper {

LayersPanel::LayersPanel(Document& document, SelectionManager& selection,
                         LayerActions& actions, const Theme& theme,
                         QWidget* parent)
    : QWidget(parent),
      document_(document),
      selection_(selection),
      actions_(actions),
      list_(new QListWidget(this)),
      add_(new QPushButton(QStringLiteral("New Layer"), this)),
      delete_(new QPushButton(QStringLiteral("Delete"), this)) {
  list_->setObjectName(QStringLiteral("layer-list"));
  list_->setItemDelegate(new LayerRowDelegate(
      theme,
      [this](Id layer, const QString& name) {
        actions_.RenameLayer(layer, name);
      },
      list_));
  list_->setEditTriggers(QAbstractItemView::DoubleClicked |
                         QAbstractItemView::EditKeyPressed |
                         QAbstractItemView::SelectedClicked);
  list_->viewport()->installEventFilter(this);

  add_->setObjectName(QStringLiteral("layer-add"));
  delete_->setObjectName(QStringLiteral("layer-delete"));
  auto* buttons = new QHBoxLayout();
  buttons->addWidget(add_);
  buttons->addWidget(delete_);
  buttons->addStretch();

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(list_);
  layout->addLayout(buttons);

  connect(add_, &QPushButton::clicked, this, [this] { actions_.AddLayer(); });
  connect(delete_, &QPushButton::clicked, this, [this] {
    QString why_not;
    actions_.DeleteLayer(selection_.ActiveLayer(), &why_not);
  });
  connect(list_, &QListWidget::currentItemChanged, this,
          [this](QListWidgetItem* item) {
            if (item != nullptr) {
              selection_.SetActiveLayer(
                  item->data(LayerRowDelegate::kIdRole).toInt());
            }
          });
  connect(&document_.Layers(), &LayerManager::ListChanged, this,
          &LayersPanel::Refill);
  connect(&document_.Layers(), &LayerManager::LayerChanged, this,
          &LayersPanel::Refill);
  connect(&selection_, &SelectionManager::ActiveLayerChanged, this,
          &LayersPanel::Refill);
  Refill();
}

void LayersPanel::RegisterCommands(CommandRegistry& commands) {
  commands.Add({
      .id = QStringLiteral("layer.rename"),
      .title = QStringLiteral("Rename Layer"),
      .shortcut = QKeySequence(Qt::Key_F2),
      .run = [this] { RenameActive(); },
  });
  commands.Add({
      .id = QStringLiteral("layer.import-picture"),
      .title = QStringLiteral("Import Picture"),
      .shortcut = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I),
      .run = [this] { ImportPicture(); },
  });
}

void LayersPanel::ImportPicture() {
  const QString path = QFileDialog::getOpenFileName(
      window(), QStringLiteral("Import Picture"), QString(),
      QStringLiteral("Pictures (*.png *.jpg *.jpeg *.webp *.bmp *.gif)"));
  if (path.isEmpty()) {
    return;
  }
  QString why_not;
  if (!actions_.ImportPicture(path, &why_not)) {
    QMessageBox::warning(
        window(), QStringLiteral("Couldn't import the picture"), why_not);
  }
}

void LayersPanel::RenameActive() {
  QListWidgetItem* item = list_->currentItem();
  if (item != nullptr) {
    list_->setFocus(Qt::ShortcutFocusReason);
    list_->editItem(item);
  }
}

bool LayersPanel::eventFilter(QObject* watched, QEvent* event) {
  const bool is_press =
      watched == list_->viewport() && event->type() == QEvent::MouseButtonPress;
  if (is_press) {
    const auto* mouse = static_cast<QMouseEvent*>(event);
    if (HandleIconPress(mouse->position().toPoint())) {
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

bool LayersPanel::HandleIconPress(const QPoint& pos) {
  const QListWidgetItem* item = list_->itemAt(pos);
  if (item == nullptr) {
    return false;
  }
  const QRect row = list_->visualItemRect(item);
  const Id layer = item->data(LayerRowDelegate::kIdRole).toInt();
  if (LayerRowDelegate::EyeRect(row).contains(pos)) {
    actions_.SetVisible(layer,
                        !item->data(LayerRowDelegate::kVisibleRole).toBool());
    return true;
  }
  if (LayerRowDelegate::LockRect(row).contains(pos)) {
    actions_.SetLocked(layer,
                       !item->data(LayerRowDelegate::kLockedRole).toBool());
    return true;
  }
  return false;
}

// Rows keep their places: the list is rebuilt in the same order and only
// the highlight moves.
void LayersPanel::Refill() {
  const QSignalBlocker blocker(list_);
  list_->clear();
  const std::vector<Layer>& layers = document_.Layers().All();
  for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
    auto* item = new QListWidgetItem(it->name, list_);
    item->setData(LayerRowDelegate::kIdRole, it->id);
    item->setData(LayerRowDelegate::kVisibleRole, it->visible);
    item->setData(LayerRowDelegate::kLockedRole, it->locked);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    if (it->id == selection_.ActiveLayer()) {
      list_->setCurrentItem(item);
    }
  }
  UpdateButtons();
}

void LayersPanel::UpdateButtons() {
  const bool can_delete = document_.Layers().Count() > 1;
  delete_->setEnabled(can_delete);
  delete_->setToolTip(can_delete
                          ? QStringLiteral("Delete the layer and its drawing")
                          : QStringLiteral("Delete Layer: a project keeps at "
                                           "least one layer"));
}

}  // namespace snapper
