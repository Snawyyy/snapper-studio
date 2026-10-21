#include "ui/LayerRowDelegate.h"

#include <QApplication>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>

namespace snapper {

namespace {

constexpr double kIconStroke = 1.5;
constexpr int kIconSize = 14;

QRect NameRect(const QRect& row) {
  return row.adjusted(LayerRowDelegate::kIconZone, 0,
                      -LayerRowDelegate::kIconZone, 0);
}

}  // namespace

LayerRowDelegate::LayerRowDelegate(
    const Theme& theme, std::function<void(Id, const QString&)> on_rename,
    QObject* parent)
    : QStyledItemDelegate(parent),
      theme_(theme),
      on_rename_(std::move(on_rename)) {}

QRect LayerRowDelegate::EyeRect(const QRect& row) {
  return {row.left(), row.top(), kIconZone, row.height()};
}

QRect LayerRowDelegate::LockRect(const QRect& row) {
  return {row.right() - kIconZone + 1, row.top(), kIconZone, row.height()};
}

void LayerRowDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const {
  QStyleOptionViewItem opt = option;
  initStyleOption(&opt, index);
  const QWidget* widget = opt.widget;
  const QStyle* style = widget != nullptr ? widget->style() : QApplication::style();
  style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);

  const bool is_visible = index.data(kVisibleRole).toBool();
  const bool is_locked = index.data(kLockedRole).toBool();
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(is_visible ? theme_.text : theme_.text_secondary);
  const QString name =
      opt.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                 Qt::ElideRight, NameRect(opt.rect).width());
  painter->drawText(NameRect(opt.rect), Qt::AlignLeft | Qt::AlignVCenter, name);
  DrawEye(*painter, EyeRect(opt.rect), is_visible);
  DrawLock(*painter, LockRect(opt.rect), is_locked);
  painter->restore();
}

QSize LayerRowDelegate::sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
  QSize size = QStyledItemDelegate::sizeHint(option, index);
  size.setHeight(std::max(size.height(), theme_.row_height));
  return size;
}

void LayerRowDelegate::setModelData(QWidget* editor,
                                    QAbstractItemModel* /*model*/,
                                    const QModelIndex& index) const {
  const auto* line = qobject_cast<QLineEdit*>(editor);
  if (line != nullptr) {
    on_rename_(index.data(kIdRole).toInt(), line->text());
  }
}

void LayerRowDelegate::updateEditorGeometry(
    QWidget* editor, const QStyleOptionViewItem& option,
    const QModelIndex& /*index*/) const {
  editor->setGeometry(NameRect(option.rect));
}

// An open eye, or a closed one when the layer is hidden.
void LayerRowDelegate::DrawEye(QPainter& painter, const QRect& zone,
                               bool visible) const {
  const QRectF box(0, 0, kIconSize, kIconSize * 0.6);
  const QRectF eye = QRectF(box).translated(zone.center() - box.center());
  painter.setPen(QPen(visible ? theme_.text : theme_.text_disabled, kIconStroke,
                      Qt::SolidLine, Qt::RoundCap));
  painter.setBrush(Qt::NoBrush);
  QPainterPath lid;
  lid.moveTo(eye.left(), eye.center().y());
  lid.quadTo(eye.center().x(), eye.bottom() + eye.height() / 2, eye.right(),
             eye.center().y());
  if (visible) {
    lid.quadTo(eye.center().x(), eye.top() - eye.height() / 2, eye.left(),
               eye.center().y());
    painter.drawPath(lid);
    painter.drawEllipse(eye.center(), eye.height() / 3, eye.height() / 3);
  } else {
    painter.drawPath(lid);
  }
}

// A shut padlock when locked, a faint open one when not.
void LayerRowDelegate::DrawLock(QPainter& painter, const QRect& zone,
                                bool locked) const {
  const double width = kIconSize * 0.7;
  const double body_height = kIconSize * 0.45;
  const QPointF center = QRectF(zone).center();
  const QRectF body(center.x() - width / 2, center.y() - body_height / 4, width,
                    body_height);
  const QRectF shackle(center.x() - width / 3.2, body.top() - width * 0.55,
                       width / 1.6, width * 1.0);
  painter.setPen(QPen(locked ? theme_.text : theme_.text_disabled, kIconStroke,
                      Qt::SolidLine, Qt::RoundCap));
  painter.setBrush(Qt::NoBrush);
  painter.drawRoundedRect(body, 1.5, 1.5);
  QPainterPath arc;
  arc.moveTo(shackle.left(), body.top());
  arc.lineTo(shackle.left(), shackle.center().y());
  arc.arcTo(shackle, 180, -180);
  const double right_end = locked ? body.top() : shackle.center().y();
  arc.lineTo(shackle.right(), right_end);
  painter.drawPath(arc);
}

}  // namespace snapper
