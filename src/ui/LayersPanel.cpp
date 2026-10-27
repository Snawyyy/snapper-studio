#include "ui/LayersPanel.h"

#include <QApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

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
      add_(new QPushButton(QStringLiteral("+ Layer"), this)),
      add_bones_(new QPushButton(QStringLiteral("+ Bones"), this)),
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
  add_->setToolTip(
      QStringLiteral("New Layer: a vector layer above the "
                     "active one"));
  add_bones_->setObjectName(QStringLiteral("layer-add-bones"));
  add_bones_->setToolTip(
      QStringLiteral("New Bone Layer: puts the active layer in a new bone "
                     "layer"));
  delete_->setObjectName(QStringLiteral("layer-delete"));
  // Three buttons share the narrow panel, so they pad less than others.
  for (QPushButton* button : {add_, add_bones_, delete_}) {
    button->setStyleSheet(QStringLiteral("padding: 6px 8px;"));
  }
  auto* buttons = new QHBoxLayout();
  buttons->addWidget(add_);
  buttons->addWidget(add_bones_);
  buttons->addWidget(delete_);
  buttons->addStretch();

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(list_);
  layout->addLayout(buttons);

  connect(add_, &QPushButton::clicked, this, [this] { actions_.AddLayer(); });
  connect(add_bones_, &QPushButton::clicked, this,
          [this] { actions_.AddBoneLayer(); });
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
  // Rows name the bone each layer follows.
  connect(&document_.Bindings(), &BindingManager::LayerBindingChanged, this,
          &LayersPanel::Refill);
  connect(&document_.Skeletons(), &SkeletonManager::BonesChanged, this,
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
  if (watched != list_->viewport()) {
    return QWidget::eventFilter(watched, event);
  }
  const bool is_press = event->type() == QEvent::MouseButtonPress;
  if (is_press) {
    const auto* mouse = static_cast<QMouseEvent*>(event);
    if (HandleIconPress(mouse->position().toPoint())) {
      return true;
    }
  }
  if (HandleDrag(event)) {
    return true;
  }
  return QWidget::eventFilter(watched, event);
}

bool LayersPanel::HandleDrag(const QEvent* event) {
  const auto* mouse = static_cast<const QMouseEvent*>(event);
  switch (event->type()) {
    case QEvent::MouseButtonPress: {
      const QListWidgetItem* item = list_->itemAt(mouse->position().toPoint());
      const bool is_left = mouse->button() == Qt::LeftButton;
      pressed_ = item != nullptr && is_left
                     ? item->data(LayerRowDelegate::kIdRole).toInt()
                     : kNoId;
      press_pos_ = mouse->position().toPoint();
      is_dragging_ = false;
      return false;
    }
    case QEvent::MouseMove: {
      const bool is_held =
          pressed_ != kNoId && mouse->buttons().testFlag(Qt::LeftButton);
      if (!is_held) {
        return false;
      }
      const bool is_past_threshold =
          (mouse->position().toPoint() - press_pos_).manhattanLength() >=
          QApplication::startDragDistance();
      if (is_past_threshold) {
        is_dragging_ = true;
      }
      if (is_dragging_) {
        ShowDropAt(mouse->position().toPoint());
      }
      return is_dragging_;
    }
    case QEvent::MouseButtonRelease: {
      const bool was_dragging = is_dragging_;
      const Id layer = pressed_;
      const Id target = drop_target_;
      const int spot = drop_spot_;
      pressed_ = kNoId;
      is_dragging_ = false;
      ClearDrop();
      const bool is_drop = was_dragging && target != kNoId && spot > 0;
      if (is_drop) {
        actions_.DropLayer(layer, target, static_cast<DropSpot>(spot - 1));
      }
      return was_dragging;
    }
    default:
      return false;
  }
}

// A bone layer row takes the layer inside below its top third; any other
// row puts it above or below, by which half it is over.
void LayersPanel::ShowDropAt(const QPoint& pos) {
  QListWidgetItem* item = list_->itemAt(pos);
  const Id target =
      item == nullptr ? kNoId : item->data(LayerRowDelegate::kIdRole).toInt();
  int spot = 0;
  const bool is_on_other = target != kNoId && target != pressed_;
  if (is_on_other) {
    const QRect row = list_->visualItemRect(item);
    const double share =
        static_cast<double>(pos.y() - row.top()) / std::max(1, row.height());
    const Layer* layer = document_.Layers().Find(target);
    const bool is_bone_layer =
        layer != nullptr && LayerManager::HoldsLayers(layer->type);
    const bool holds_none = document_.Layers().EndOf(target) ==
                            document_.Layers().IndexOf(target) + 1;
    DropSpot drop = share < 0.5 ? DropSpot::kAbove : DropSpot::kBelow;
    if (is_bone_layer) {
      const bool is_bottom_edge = holds_none && share > 0.75;
      drop = share < 0.3      ? DropSpot::kAbove
             : is_bottom_edge ? DropSpot::kBelow
                              : DropSpot::kInto;
    }
    spot = static_cast<int>(drop) + 1;
  }
  if (target == drop_target_ && spot == drop_spot_) {
    return;
  }
  ClearDrop();
  drop_target_ = target;
  drop_spot_ = spot;
  if (item != nullptr && spot > 0) {
    item->setData(LayerRowDelegate::kDropRole, spot);
  }
}

void LayersPanel::ClearDrop() {
  for (int i = 0; i < list_->count(); ++i) {
    list_->item(i)->setData(LayerRowDelegate::kDropRole, 0);
  }
  drop_target_ = kNoId;
  drop_spot_ = 0;
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
  const LayerManager& layers = document_.Layers();
  for (const int index : layers.TopFirst()) {
    const Layer& layer = layers.All()[static_cast<size_t>(index)];
    auto* item = new QListWidgetItem(layer.name, list_);
    item->setData(LayerRowDelegate::kIdRole, layer.id);
    item->setData(LayerRowDelegate::kVisibleRole, layer.visible);
    item->setData(LayerRowDelegate::kLockedRole, layer.locked);
    item->setData(LayerRowDelegate::kDepthRole, layers.DepthOf(layer.id));
    item->setData(LayerRowDelegate::kIsBoneLayerRole,
                  layer.type == LayerType::kBone);
    const Bone* bone =
        document_.Skeletons().Find(document_.Bindings().BoneOfLayer(layer.id));
    const bool is_bound =
        bone != nullptr && document_.CanBind(layer.id, bone->id);
    if (is_bound) {
      item->setData(LayerRowDelegate::kBoundRole, bone->name);
    }
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    if (layer.id == selection_.ActiveLayer()) {
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
