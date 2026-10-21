#include "ui/StyleEditor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include <initializer_list>

#include "document/Document.h"
#include "edit/LookManager.h"
#include "edit/StyleActions.h"
#include "ui/ColorPopover.h"
#include "ui/ColorSwatch.h"
#include "ui/LookCombos.h"
#include "ui/NumberField.h"

namespace snapper {

namespace {

constexpr int kFieldWidth = 72;

}  // namespace

StyleEditor::StyleEditor(Document& document, StyleActions& actions,
                         ColorPopover& popover, QWidget* parent)
    : QWidget(parent),
      document_(document),
      actions_(actions),
      popover_(popover),
      users_(new QLabel(this)),
      name_(new QLineEdit(this)),
      sets_fill_(new QCheckBox(QStringLiteral("Fill"), this)),
      fill_(new ColorSwatch(popover, this)),
      no_fill_(new QCheckBox(QStringLiteral("None"), this)),
      sets_line_(new QCheckBox(QStringLiteral("Line"), this)),
      line_(new ColorSwatch(popover, this)),
      no_line_(new QCheckBox(QStringLiteral("None"), this)),
      sets_width_(new QCheckBox(QStringLiteral("Width"), this)),
      width_(new NumberField(this)),
      sets_brush_(new QCheckBox(QStringLiteral("Brush"), this)),
      brush_(new QComboBox(this)) {
  users_->setObjectName(QStringLiteral("secondary"));
  name_->setObjectName(QStringLiteral("style-name"));
  fill_->setObjectName(QStringLiteral("style-fill"));
  line_->setObjectName(QStringLiteral("style-line"));
  width_->setObjectName(QStringLiteral("style-width"));
  brush_->setObjectName(QStringLiteral("style-brush"));
  width_->setRange(LookManager::kMinLineWidth, LookManager::kMaxLineWidth);
  width_->setDecimals(1);
  width_->setSingleStep(0.5);
  width_->setSuffix(QStringLiteral(" px"));
  width_->setFixedWidth(kFieldWidth);
  name_->setPlaceholderText(QStringLiteral("Style name"));
  for (QCheckBox* part : {sets_fill_, sets_line_, sets_width_, sets_brush_}) {
    part->setToolTip(
        QStringLiteral("Ticked: the style sets this. Unticked: each shape "
                       "keeps its own."));
  }

  auto* grid = new QGridLayout();
  grid->setHorizontalSpacing(8);
  grid->setVerticalSpacing(6);
  grid->addWidget(sets_fill_, 0, 0);
  grid->addWidget(fill_, 0, 1);
  grid->addWidget(no_fill_, 0, 2);
  grid->addWidget(sets_line_, 1, 0);
  grid->addWidget(line_, 1, 1);
  grid->addWidget(no_line_, 1, 2);
  grid->addWidget(sets_width_, 2, 0);
  grid->addWidget(width_, 2, 1, 1, 2, Qt::AlignLeft);
  grid->addWidget(sets_brush_, 3, 0);
  grid->addWidget(brush_, 3, 1, 1, 2, Qt::AlignLeft);
  grid->setColumnStretch(3, 1);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(name_);
  layout->addWidget(users_);
  layout->addLayout(grid);

  connect(name_, &QLineEdit::editingFinished, this,
          [this] { actions_.RenameStyle(style_, name_->text()); });
  const auto part_toggle = [this](QCheckBox* box, LookPart part) {
    connect(box, &QCheckBox::toggled, this, [this, part](bool sets) {
      Edit([&](Style& style) { style.parts.Set(part, sets); });
    });
  };
  part_toggle(sets_fill_, LookPart::kFill);
  part_toggle(sets_line_, LookPart::kLine);
  part_toggle(sets_width_, LookPart::kWidth);
  part_toggle(sets_brush_, LookPart::kBrush);

  connect(fill_, &ColorSwatch::ColorChanged, this,
          [this](const QColor& color, int tag) {
            Edit(
                [&](Style& style) {
                  style.parts.fill = true;
                  style.look.has_fill = true;
                  style.look.fill = color;
                },
                tag);
          });
  connect(no_fill_, &QCheckBox::toggled, this, [this](bool none) {
    Edit([none](Style& style) {
      style.parts.fill = true;
      style.look.has_fill = !none;
    });
  });
  connect(line_, &ColorSwatch::ColorChanged, this,
          [this](const QColor& color, int tag) {
            Edit(
                [&](Style& style) {
                  style.parts.line = true;
                  style.look.has_line = true;
                  style.look.line = color;
                },
                tag);
          });
  connect(no_line_, &QCheckBox::toggled, this, [this](bool none) {
    Edit([none](Style& style) {
      style.parts.line = true;
      style.look.has_line = !none;
    });
  });
  connect(width_, &NumberField::valueChanged, this, [this](double width) {
    Edit(
        [width](Style& style) {
          style.parts.width = true;
          style.look.line_width = static_cast<float>(width);
        },
        width_drag_.Tag());
  });
  connect(brush_, &QComboBox::activated, this, [this] {
    const Id brush = PickedId(*brush_);
    Edit([brush](Style& style) {
      style.parts.brush = true;
      style.look.brush = brush;
    });
  });

  const auto refresh = [this] { Refresh(); };
  connect(&document_.Styles(), &StyleManager::StyleChanged, this, refresh);
  connect(&document_.Styles(), &StyleManager::ListChanged, this, refresh);
  connect(&document_.Brushes(), &BrushManager::ListChanged, this, refresh);
  connect(&document_.Brushes(), &BrushManager::BrushChanged, this, refresh);
  connect(&document_.Shapes(), &ShapeManager::LayerContentChanged, this,
          refresh);
  Refresh();
}

bool StyleEditor::IsPicking() const {
  return popover_.IsOpenFor(fill_) || popover_.IsOpenFor(line_);
}

void StyleEditor::SetStyle(Id style) {
  style_ = style;
  Refresh();
}

void StyleEditor::Edit(const std::function<void(Style&)>& edit, int drag_tag) {
  actions_.EditStyle(style_, edit, drag_tag);
}

void StyleEditor::Refresh() {
  const Style* style = document_.Styles().Find(style_);
  setEnabled(style != nullptr);
  const Style shown = style != nullptr ? *style : Style{};
  const std::initializer_list<QWidget*> controls = {
      name_,    sets_fill_,  no_fill_, sets_line_,
      no_line_, sets_width_, width_,   sets_brush_};
  for (QWidget* control : controls) {
    control->blockSignals(true);
  }
  if (!name_->hasFocus()) {
    name_->setText(shown.name);
  }
  const int users =
      static_cast<int>(document_.Shapes().ShapesWithStyle(style_).size());
  users_->setText(style == nullptr ? QStringLiteral("Pick a style to edit it")
                  : users == 1
                      ? QStringLiteral("Used by 1 shape")
                      : QStringLiteral("Used by %1 shapes").arg(users));
  sets_fill_->setChecked(shown.parts.fill);
  fill_->SetColor(shown.look.fill);
  no_fill_->setChecked(!shown.look.has_fill);
  sets_line_->setChecked(shown.parts.line);
  line_->SetColor(shown.look.line);
  no_line_->setChecked(!shown.look.has_line);
  sets_width_->setChecked(shown.parts.width);
  width_->setValue(shown.look.line_width);
  sets_brush_->setChecked(shown.parts.brush);
  FillBrushCombo(*brush_, document_.Brushes(), shown.look.brush);
  for (QWidget* control : controls) {
    control->blockSignals(false);
  }
}

}  // namespace snapper
