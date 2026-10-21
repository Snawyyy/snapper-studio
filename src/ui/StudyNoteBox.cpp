#include "ui/StudyNoteBox.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

#include "app/StudyManager.h"

namespace snapper {

namespace {

constexpr int kWidth = 340;
constexpr int kLevels = 5;
constexpr int kLevelButtonSize = 32;
constexpr int kEdgeGap = 8;

}  // namespace

StudyNoteBox::StudyNoteBox(StudyManager& study, QWidget* parent)
    : QFrame(parent),
      study_(study),
      text_(new QLineEdit(this)),
      frustration_(new QButtonGroup(this)) {
  setObjectName(QStringLiteral("study-note"));
  setAttribute(Qt::WA_StyledBackground);
  setFixedWidth(kWidth);
  text_->setPlaceholderText(QStringLiteral("What are you trying to do?"));

  auto* levels = new QHBoxLayout();
  auto* label = new QLabel(QStringLiteral("Frustration"), this);
  levels->addWidget(label);
  for (int level = 1; level <= kLevels; ++level) {
    auto* button = new QPushButton(QString::number(level), this);
    button->setCheckable(true);
    button->setFixedSize(kLevelButtonSize, kLevelButtonSize);
    button->setFocusPolicy(Qt::NoFocus);
    frustration_->addButton(button, level);
    levels->addWidget(button);
  }
  levels->addStretch();
  auto* scale = new QLabel(
      QStringLiteral("1 calm, 5 stuck (Alt+1 to Alt+5). Enter saves."), this);
  scale->setObjectName(QStringLiteral("secondary"));

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(text_);
  layout->addLayout(levels);
  layout->addWidget(scale);
  connect(text_, &QLineEdit::returnPressed, this, &StudyNoteBox::SaveNote);
  text_->installEventFilter(this);
  hide();
}

void StudyNoteBox::Open() {
  spot_ = study_.RestSpot();
  text_->clear();
  frustration_->setExclusive(false);
  for (QAbstractButton* button : frustration_->buttons()) {
    button->setChecked(false);
  }
  frustration_->setExclusive(true);
  adjustSize();
  // Next to the spot, but never past the window's edge.
  const QWidget* host = parentWidget();
  const int x = std::clamp(spot_.x() + kEdgeGap, kEdgeGap,
                           host->width() - width() - kEdgeGap);
  const int y = std::clamp(spot_.y() + kEdgeGap, kEdgeGap,
                           host->height() - height() - kEdgeGap);
  move(x, y);
  show();
  raise();
  text_->setFocus(Qt::ShortcutFocusReason);
}

void StudyNoteBox::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Escape) {
    hide();
    return;
  }
  QFrame::keyPressEvent(event);
}

// Alt+1 to Alt+5 pick the level while typing, before the text field can
// take the digit.
bool StudyNoteBox::eventFilter(QObject* watched, QEvent* event) {
  const bool is_key = watched == text_ && event->type() == QEvent::KeyPress;
  if (is_key) {
    const auto* key = static_cast<const QKeyEvent*>(event);
    const int level = key->key() - Qt::Key_0;
    const bool is_level_key = key->modifiers().testFlag(Qt::AltModifier) &&
                              level >= 1 && level <= kLevels;
    if (is_level_key) {
      frustration_->button(level)->setChecked(true);
      return true;
    }
  }
  return QFrame::eventFilter(watched, event);
}

// 0 means no level was picked.
void StudyNoteBox::SaveNote() {
  const int level = std::max(0, frustration_->checkedId());
  study_.RecordNote(text_->text(), level, spot_);
  hide();
}

}  // namespace snapper
