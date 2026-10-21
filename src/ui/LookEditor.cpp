#include "ui/LookEditor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>

#include <initializer_list>

#include "document/Document.h"
#include "edit/LookManager.h"
#include "ui/ColorSwatch.h"
#include "ui/LookCombos.h"
#include "ui/NumberField.h"

namespace snapper {

namespace {

constexpr int kFieldWidth = 72;

QLabel* Note(QWidget* parent) {
  auto* note = new QLabel(parent);
  note->setObjectName(QStringLiteral("secondary"));
  return note;
}

}  // namespace

LookEditor::LookEditor(Document& document, LookManager& looks,
                       ColorPopover& popover, QWidget* parent)
    : QWidget(parent),
      document_(document),
      looks_(looks),
      title_(new QLabel(this)),
      fill_on_(new QCheckBox(QStringLiteral("Fill"), this)),
      fill_(new ColorSwatch(popover, this)),
      fill_note_(Note(this)),
      line_on_(new QCheckBox(QStringLiteral("Line"), this)),
      line_(new ColorSwatch(popover, this)),
      line_note_(Note(this)),
      width_(new NumberField(this)),
      width_note_(Note(this)),
      brush_(new QComboBox(this)),
      brush_note_(Note(this)),
      style1_(new QComboBox(this)),
      style2_(new QComboBox(this)) {
  title_->setObjectName(QStringLiteral("section-title"));
  fill_->setObjectName(QStringLiteral("look-fill"));
  line_->setObjectName(QStringLiteral("look-line"));
  width_->setObjectName(QStringLiteral("look-width"));
  brush_->setObjectName(QStringLiteral("look-brush"));
  style1_->setObjectName(QStringLiteral("look-style1"));
  style2_->setObjectName(QStringLiteral("look-style2"));
  width_->setRange(LookManager::kMinLineWidth, LookManager::kMaxLineWidth);
  width_->setDecimals(1);
  width_->setSingleStep(0.5);
  width_->setSuffix(QStringLiteral(" px"));
  width_->setFixedWidth(kFieldWidth);

  // Labels sit right next to their values, each row: label, value, note.
  auto* grid = new QGridLayout();
  grid->setHorizontalSpacing(8);
  grid->setVerticalSpacing(6);
  grid->addWidget(fill_on_, 0, 0);
  grid->addWidget(fill_, 0, 1);
  grid->addWidget(fill_note_, 0, 2);
  grid->addWidget(line_on_, 1, 0);
  grid->addWidget(line_, 1, 1);
  grid->addWidget(line_note_, 1, 2);
  grid->addWidget(new QLabel(QStringLiteral("Width"), this), 2, 0);
  grid->addWidget(width_, 2, 1);
  grid->addWidget(width_note_, 2, 2);
  grid->addWidget(new QLabel(QStringLiteral("Brush"), this), 3, 0);
  grid->addWidget(brush_, 3, 1);
  grid->addWidget(brush_note_, 3, 2);
  grid->addWidget(new QLabel(QStringLiteral("Style 1"), this), 4, 0);
  grid->addWidget(style1_, 4, 1);
  grid->addWidget(new QLabel(QStringLiteral("Style 2"), this), 5, 0);
  grid->addWidget(style2_, 5, 1);
  grid->setColumnStretch(3, 1);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(title_);
  layout->addLayout(grid);

  connect(fill_on_, &QCheckBox::toggled, this, [this](bool on) {
    looks_.Change(on ? QStringLiteral("Fill On") : QStringLiteral("Fill Off"),
                  [on](ShapeLookState& state) { state.look.has_fill = on; });
  });
  connect(fill_, &ColorSwatch::ColorChanged, this,
          [this](const QColor& color, int tag) {
            looks_.Change(
                QStringLiteral("Fill Color"),
                [&](ShapeLookState& state) {
                  state.look.fill = color;
                  state.look.has_fill = true;
                },
                tag);
          });
  connect(line_on_, &QCheckBox::toggled, this, [this](bool on) {
    looks_.Change(on ? QStringLiteral("Line On") : QStringLiteral("Line Off"),
                  [on](ShapeLookState& state) { state.look.has_line = on; });
  });
  connect(line_, &ColorSwatch::ColorChanged, this,
          [this](const QColor& color, int tag) {
            looks_.Change(
                QStringLiteral("Line Color"),
                [&](ShapeLookState& state) {
                  state.look.line = color;
                  state.look.has_line = true;
                },
                tag);
          });
  connect(width_, &NumberField::valueChanged, this, [this](double width) {
    looks_.Change(
        QStringLiteral("Line Width"),
        [width](ShapeLookState& state) {
          state.look.line_width = static_cast<float>(width);
        },
        width_drag_.Tag());
  });
  connect(brush_, &QComboBox::activated, this, [this] {
    const Id brush = PickedId(*brush_);
    looks_.Change(QStringLiteral("Brush"),
                  [brush](ShapeLookState& state) { state.look.brush = brush; });
  });
  connect(style1_, &QComboBox::activated, this, [this] {
    const Id style = PickedId(*style1_);
    looks_.Change(QStringLiteral("Style 1"),
                  [style](ShapeLookState& state) { state.styles[0] = style; });
  });
  connect(style2_, &QComboBox::activated, this, [this] {
    const Id style = PickedId(*style2_);
    looks_.Change(QStringLiteral("Style 2"),
                  [style](ShapeLookState& state) { state.styles[1] = style; });
  });

  const auto refresh = [this] { Refresh(); };
  connect(&looks_, &LookManager::ShownChanged, this, refresh);
  connect(&document_.Brushes(), &BrushManager::ListChanged, this, refresh);
  connect(&document_.Brushes(), &BrushManager::BrushChanged, this, refresh);
  connect(&document_.Styles(), &StyleManager::ListChanged, this, refresh);
  connect(&document_.Styles(), &StyleManager::StyleChanged, this, refresh);
  Refresh();
}

void LookEditor::Refresh() {
  const std::vector<Id> selected = looks_.SelectedShapes();
  const int count = static_cast<int>(selected.size());
  title_->setText(count == 0   ? QStringLiteral("New shapes")
                  : count == 1 ? QStringLiteral("1 shape")
                               : QStringLiteral("%1 shapes").arg(count));

  // Shown as it draws, so a part a style sets shows the style's value.
  const ShapeLookState shown = looks_.Shown();
  const ShapeLook look = looks_.ShownResolved();
  for (QWidget* control : std::initializer_list<QWidget*>{
           fill_on_, line_on_, width_, brush_, style1_, style2_}) {
    control->blockSignals(true);
  }
  fill_on_->setChecked(look.has_fill);
  fill_->SetColor(look.fill);
  line_on_->setChecked(look.has_line);
  line_->SetColor(look.line);
  width_->setValue(look.line_width);
  FillBrushCombo(*brush_, document_.Brushes(), look.brush);
  FillStyleCombo(*style1_, document_.Styles(), shown.styles[0]);
  FillStyleCombo(*style2_, document_.Styles(), shown.styles[1]);
  for (QWidget* control : std::initializer_list<QWidget*>{
           fill_on_, line_on_, width_, brush_, style1_, style2_}) {
    control->blockSignals(false);
  }

  ShowSource(LookPart::kFill, fill_on_, fill_note_);
  fill_->setEnabled(fill_on_->isEnabled());
  ShowSource(LookPart::kLine, line_on_, line_note_);
  line_->setEnabled(line_on_->isEnabled());
  ShowSource(LookPart::kWidth, width_, width_note_);
  ShowSource(LookPart::kBrush, brush_, brush_note_);
}

void LookEditor::ShowSource(LookPart part, QWidget* control, QLabel* note) {
  const QString style = looks_.StyleSetting(part);
  const bool is_from_style = !style.isEmpty();
  control->setEnabled(!is_from_style);
  note->setText(is_from_style ? QStringLiteral("from %1").arg(style)
                              : QString());
  control->setToolTip(
      is_from_style
          ? QStringLiteral("Set by the style %1: edit it in the style list")
                .arg(style)
          : QString());
}

}  // namespace snapper
