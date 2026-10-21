#include "ui/CommandBox.h"

#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

#include "app/CommandRegistry.h"

namespace snapper {

namespace {

constexpr int kWidth = 520;
constexpr int kTopMargin = 48;
constexpr int kVisibleRows = 10;
constexpr int kPadding = 8;
constexpr int kFallbackRowHeight = 28;
constexpr int kIdRole = Qt::UserRole;
constexpr int kShortcutRole = Qt::UserRole + 1;

// Draws the row as usual, then its shortcut in secondary text on the right.
class ShortcutDelegate : public QStyledItemDelegate {
 public:
  using QStyledItemDelegate::QStyledItemDelegate;

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override {
    QStyledItemDelegate::paint(painter, option, index);
    const QString shortcut = index.data(kShortcutRole).toString();
    if (shortcut.isEmpty()) {
      return;
    }
    painter->save();
    painter->setPen(option.palette.color(QPalette::PlaceholderText));
    painter->drawText(option.rect.adjusted(0, 0, -12, 0),
                      Qt::AlignRight | Qt::AlignVCenter, shortcut);
    painter->restore();
  }
};

}  // namespace

CommandBox::CommandBox(CommandRegistry* registry, QWidget* parent)
    : QFrame(parent),
      registry_(registry),
      search_(new QLineEdit(this)),
      results_(new QListWidget(this)) {
  setObjectName(QStringLiteral("command-box"));
  setAttribute(Qt::WA_StyledBackground);
  search_->setObjectName(QStringLiteral("command-search"));
  search_->setPlaceholderText(QStringLiteral("Commands (Ctrl+K)"));
  results_->setObjectName(QStringLiteral("command-results"));
  results_->setItemDelegate(new ShortcutDelegate(results_));
  results_->setFocusPolicy(Qt::NoFocus);
  results_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(kPadding, kPadding, kPadding, kPadding);
  layout->setSpacing(kPadding);
  layout->addWidget(search_);
  layout->addWidget(results_);

  auto* shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(32);
  shadow->setOffset(0, 8);
  shadow->setColor(QColor(0, 0, 0, 160));
  setGraphicsEffect(shadow);

  connect(search_, &QLineEdit::textChanged, this, [this] { Refill(); });
  connect(results_, &QListWidget::itemClicked, this, [this] { RunCurrent(); });
  search_->installEventFilter(this);
  parent->installEventFilter(this);
  hide();
}

void CommandBox::Open() {
  search_->clear();
  Refill();
  PlaceInParent();
  show();
  raise();
  search_->setFocus();
}

void CommandBox::Close() {
  hide();
  if (parentWidget() != nullptr) {
    parentWidget()->setFocus();
  }
}

// Arrow keys move through results while typing stays in the search field.
// Also follows the parent's size so the box stays inside the window.
bool CommandBox::eventFilter(QObject* watched, QEvent* event) {
  const bool is_parent_resize =
      watched == parentWidget() && event->type() == QEvent::Resize;
  if (is_parent_resize && isVisible()) {
    PlaceInParent();
  }

  const bool is_search_key =
      watched == search_ && event->type() == QEvent::KeyPress;
  if (!is_search_key) {
    return QFrame::eventFilter(watched, event);
  }

  const auto* key_event = static_cast<QKeyEvent*>(event);
  const int row = results_->currentRow();
  const int last_row = results_->count() - 1;
  switch (key_event->key()) {
    case Qt::Key_Escape:
      Close();
      return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
      RunCurrent();
      return true;
    case Qt::Key_Down:
      results_->setCurrentRow(std::min(row + 1, last_row));
      return true;
    case Qt::Key_Up:
      results_->setCurrentRow(std::max(row - 1, 0));
      return true;
    default:
      return QFrame::eventFilter(watched, event);
  }
}

void CommandBox::Refill() {
  results_->clear();
  const std::vector<QString> ids = registry_->Search(search_->text());
  for (const QString& id : ids) {
    const Command* command = registry_->Find(id);
    auto* item = new QListWidgetItem(command->title, results_);
    item->setData(kIdRole, id);
    item->setData(kShortcutRole, command->shortcut.toString(
                                     QKeySequence::NativeText));
  }
  if (results_->count() > 0) {
    results_->setCurrentRow(0);
  }
  PlaceInParent();
}

// Closes first so the command sees the window as it was before Ctrl+K.
void CommandBox::RunCurrent() {
  const QListWidgetItem* item = results_->currentItem();
  if (item == nullptr) {
    return;
  }
  const QString id = item->data(kIdRole).toString();
  Close();
  registry_->Run(id);
}

void CommandBox::PlaceInParent() {
  const QWidget* parent = parentWidget();
  const int hinted_height = results_->sizeHintForRow(0);
  const bool has_rows = hinted_height > 0;
  const int row_height = has_rows ? hinted_height : kFallbackRowHeight;
  const int rows = std::min(results_->count(), kVisibleRows);
  results_->setFixedHeight(rows * row_height + 2);
  results_->setVisible(rows > 0);

  const int width = std::min(kWidth, parent->width() - 2 * kTopMargin);
  const int height = sizeHint().height();
  const int left = (parent->width() - width) / 2;
  setGeometry(left, kTopMargin, width, height);
}

}  // namespace snapper
