#pragma once

#include <QStyledItemDelegate>

#include <functional>

#include "app/Theme.h"
#include "document/Ids.h"

namespace snapper {

// Draws one row of the Layers panel: an eye to show or hide the layer, its
// name, and a padlock. Renaming edits the name in place.
class LayerRowDelegate : public QStyledItemDelegate {
  Q_OBJECT

 public:
  static constexpr int kIdRole = Qt::UserRole;
  static constexpr int kVisibleRole = Qt::UserRole + 1;
  static constexpr int kLockedRole = Qt::UserRole + 2;
  static constexpr int kIconZone = 28;

  LayerRowDelegate(const Theme& theme,
                   std::function<void(Id, const QString&)> on_rename,
                   QObject* parent = nullptr);

  // The eye and padlock spots of a row, for clicks.
  static QRect EyeRect(const QRect& row);
  static QRect LockRect(const QRect& row);

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
  void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                            const QModelIndex& index) const override;

 private:
  void DrawEye(QPainter& painter, const QRect& zone, bool visible) const;
  void DrawLock(QPainter& painter, const QRect& zone, bool locked) const;

  const Theme& theme_;
  std::function<void(Id, const QString&)> on_rename_;
};

}  // namespace snapper
