#include "ui/EyedropperOptions.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace snapper {

EyedropperOptions::EyedropperOptions(EyedropperTool& tool, QWidget* parent)
    : QWidget(parent),
      tool_(tool),
      color_(new QPushButton(QStringLiteral("Color"), this)),
      style_(new QPushButton(QStringLiteral("Whole style"), this)) {
  auto* group = new QButtonGroup(this);
  for (QPushButton* button : {color_, style_}) {
    button->setCheckable(true);
    group->addButton(button);
  }
  color_->setObjectName(QStringLiteral("eyedropper-color"));
  style_->setObjectName(QStringLiteral("eyedropper-style"));
  color_->setToolTip(QStringLiteral(
      "Pick the line color from a line, the fill color from a fill, or a "
      "picture's color as the line color (I)"));
  style_->setToolTip(
      QStringLiteral("Pick a shape's whole look and styles (Alt+I)"));

  auto* buttons = new QHBoxLayout();
  buttons->addWidget(color_);
  buttons->addWidget(style_);
  buttons->addStretch();
  auto* note = new QLabel(
      QStringLiteral("Alt+click picks the other way. Picks go to the "
                     "selected shapes, or to new shapes."),
      this);
  note->setObjectName(QStringLiteral("secondary"));
  note->setWordWrap(true);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(new QLabel(QStringLiteral("Pick"), this));
  layout->addLayout(buttons);
  layout->addWidget(note);
  layout->addStretch();

  connect(color_, &QPushButton::clicked, this,
          [this] { tool_.SetMode(EyedropperTool::Mode::kColor); });
  connect(style_, &QPushButton::clicked, this,
          [this] { tool_.SetMode(EyedropperTool::Mode::kWholeStyle); });
  Refresh();
}

void EyedropperOptions::Refresh() {
  const bool is_color = tool_.CurrentMode() == EyedropperTool::Mode::kColor;
  color_->setChecked(is_color);
  style_->setChecked(!is_color);
}

}  // namespace snapper
