#include "ui/ShapeOptions.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QVBoxLayout>

#include "ui/NumberField.h"

namespace snapper {

namespace {

constexpr int kFieldWidth = 72;

}  // namespace

ShapeOptions::ShapeOptions(
    ShapeTool& tool, std::function<void(const ShapeToolSettings&)> on_change,
    QWidget* parent)
    : QWidget(parent),
      tool_(tool),
      on_change_(std::move(on_change)),
      fill_(new QCheckBox(QStringLiteral("Fill"), this)),
      line_(new QCheckBox(QStringLiteral("Line"), this)),
      width_(new NumberField(this)) {
  const ShapeToolSettings& settings = tool.Settings();
  fill_->setChecked(settings.has_fill);
  line_->setChecked(settings.has_line);
  width_->setRange(0.5, 200);
  width_->setDecimals(1);
  width_->setSingleStep(0.5);
  width_->setSuffix(QStringLiteral(" px"));
  width_->setValue(settings.line_width);
  width_->setFixedWidth(kFieldWidth);

  auto* form = new QFormLayout();
  form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
  form->addRow(QStringLiteral("Line width"), width_);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(fill_);
  layout->addWidget(line_);
  layout->addLayout(form);
  layout->addStretch();

  connect(width_, &NumberField::valueChanged, this, [this] { Apply(); });
  for (const QCheckBox* box : {fill_, line_}) {
    connect(box, &QCheckBox::toggled, this, [this] { Apply(); });
  }
}

void ShapeOptions::Apply() {
  tool_.SetSettings({.has_fill = fill_->isChecked(),
                     .has_line = line_->isChecked(),
                     .line_width = width_->value()});
  on_change_(tool_.Settings());
}

}  // namespace snapper
