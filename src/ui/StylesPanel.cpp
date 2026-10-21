#include "ui/StylesPanel.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "document/Document.h"
#include "edit/StyleActions.h"
#include "ui/ColorPopover.h"
#include "ui/LookEditor.h"
#include "ui/StyleEditor.h"

namespace snapper {

namespace {

constexpr int kIconSize = 18;
constexpr int kListHeight = 140;

// A small rounded patch: the style's fill, ringed by its line color. A
// part it doesn't set shows as a dashed gap.
QIcon StyleIcon(const Style& style, const QColor& empty) {
  QPixmap pixmap(kIconSize * 2, kIconSize * 2);
  pixmap.setDevicePixelRatio(2.0);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  const QRectF area(2, 2, kIconSize - 4, kIconSize - 4);
  const bool has_fill = style.parts.fill && style.look.has_fill;
  painter.setBrush(has_fill ? QBrush(style.look.fill) : QBrush(Qt::NoBrush));
  const bool has_line = style.parts.line && style.look.has_line;
  QPen pen(has_line ? style.look.line : empty, 2.0);
  if (!style.parts.line) {
    pen.setStyle(Qt::DotLine);
  }
  painter.setPen(pen);
  painter.drawRoundedRect(area, 4, 4);
  return QIcon(pixmap);
}

QLabel* Title(const QString& text, QWidget* parent) {
  auto* label = new QLabel(text, parent);
  label->setObjectName(QStringLiteral("section-title"));
  return label;
}

}  // namespace

StylesPanel::StylesPanel(Document& document, LookManager& looks,
                         StyleActions& actions, ColorPopover& popover,
                         const Theme& theme, QWidget* parent)
    : QWidget(parent),
      document_(document),
      actions_(actions),
      popover_(popover),
      theme_(theme),
      styles_area_(new QWidget()),
      list_(new QListWidget(styles_area_)),
      add_(new QPushButton(QStringLiteral("New Style"), styles_area_)),
      delete_(new QPushButton(QStringLiteral("Delete"), styles_area_)),
      editor_(new StyleEditor(document, actions, popover, styles_area_)) {
  list_->setObjectName(QStringLiteral("style-list"));
  list_->setIconSize(QSize(kIconSize, kIconSize));
  list_->setFixedHeight(kListHeight);
  add_->setObjectName(QStringLiteral("style-add"));
  add_->setToolTip(
      QStringLiteral("Make a style from the look above and use it as Style 1"));
  delete_->setObjectName(QStringLiteral("style-delete"));

  auto* buttons = new QHBoxLayout();
  buttons->addWidget(add_);
  buttons->addWidget(delete_);
  buttons->addStretch();

  auto* styles_layout = new QVBoxLayout(styles_area_);
  styles_layout->setContentsMargins(0, 0, 0, 0);
  styles_layout->addWidget(Title(QStringLiteral("Styles"), styles_area_));
  styles_layout->addWidget(list_);
  styles_layout->addLayout(buttons);
  styles_layout->addSpacing(8);
  styles_layout->addWidget(editor_);
  styles_area_->installEventFilter(this);

  auto* separator = new QFrame();
  separator->setObjectName(QStringLiteral("separator"));
  separator->setFrameShape(QFrame::HLine);

  auto* content = new QWidget();
  auto* content_layout = new QVBoxLayout(content);
  content_layout->setContentsMargins(0, 0, 0, 0);
  content_layout->setSpacing(12);
  content_layout->addWidget(new LookEditor(document, looks, popover, content));
  content_layout->addWidget(separator);
  content_layout->addWidget(styles_area_);
  content_layout->addStretch();

  auto* scroll = new QScrollArea(this);
  scroll->setObjectName(QStringLiteral("styles-scroll"));
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll->setWidget(content);
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(scroll);

  connect(add_, &QPushButton::clicked, this, [this] {
    const Id style = actions_.NewStyle();
    if (style != kNoId) {
      editor_->SetStyle(style);
      Refill();
    }
  });
  connect(delete_, &QPushButton::clicked, this,
          [this] { actions_.DeleteStyle(editor_->CurrentStyle()); });
  connect(list_, &QListWidget::currentItemChanged, this,
          [this](QListWidgetItem* item) {
            if (item != nullptr) {
              editor_->SetStyle(item->data(Qt::UserRole).toInt());
              UpdateButtons();
              UpdateHighlight();
            }
          });
  connect(&popover_, &ColorPopover::Closed, this,
          &StylesPanel::UpdateHighlight);
  const auto refill = [this] { Refill(); };
  connect(&document_.Styles(), &StyleManager::ListChanged, this, refill);
  connect(&document_.Styles(), &StyleManager::StyleChanged, this, refill);
  connect(&document_.Shapes(), &ShapeManager::LayerContentChanged, this,
          [this] { UpdateButtons(); });
  connect(&document_, &Document::Reset, this, refill);
  Refill();
}

bool StylesPanel::eventFilter(QObject* watched, QEvent* event) {
  if (watched == styles_area_) {
    const bool is_enter = event->type() == QEvent::Enter;
    const bool is_leave = event->type() == QEvent::Leave;
    if (is_enter || is_leave) {
      is_pointer_over_styles_ = is_enter;
      UpdateHighlight();
    }
  }
  return QWidget::eventFilter(watched, event);
}

// Rows keep their places: the list is rebuilt in the same order and only
// the highlight moves. A deleted style's place goes to the one before it.
void StylesPanel::Refill() {
  const QSignalBlocker blocker(list_);
  const Id current = editor_->CurrentStyle();
  const int old_row = list_->currentRow();
  list_->clear();
  const std::vector<Style>& styles = document_.Styles().All();
  for (const Style& style : styles) {
    auto* item = new QListWidgetItem(StyleIcon(style, theme_.text_secondary),
                                     style.name, list_);
    item->setData(Qt::UserRole, style.id);
    if (style.id == current) {
      list_->setCurrentItem(item);
    }
  }
  const bool is_lost = list_->currentItem() == nullptr && list_->count() > 0;
  if (is_lost) {
    list_->setCurrentRow(std::clamp(old_row, 0, list_->count() - 1));
  }
  const QListWidgetItem* item = list_->currentItem();
  editor_->SetStyle(item == nullptr ? kNoId : item->data(Qt::UserRole).toInt());
  UpdateButtons();
  UpdateHighlight();
}

void StylesPanel::UpdateButtons() {
  const Id style = editor_->CurrentStyle();
  const bool can_delete = document_.Styles().Find(style) != nullptr;
  delete_->setEnabled(can_delete);
  delete_->setToolTip(can_delete
                          ? actions_.DeleteEffect(style)
                          : QStringLiteral("Delete Style: pick a style first"));
}

// Shown while the pointer is over the styles, or a style color is being
// picked.
void StylesPanel::UpdateHighlight() {
  const bool is_editing = is_pointer_over_styles_ || editor_->IsPicking();
  Q_EMIT HighlightStyle(is_editing ? editor_->CurrentStyle() : kNoId);
}

}  // namespace snapper
