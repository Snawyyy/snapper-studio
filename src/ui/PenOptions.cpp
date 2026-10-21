#include "ui/PenOptions.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QVBoxLayout>

#include "ui/NumberField.h"

namespace snapper {

namespace {

constexpr int kFieldWidth = 72;

}  // namespace

PenOptions::PenOptions(PenTool& pen,
                       std::function<void(const PenSettings&)> on_change,
                       QWidget* parent)
    : QWidget(parent),
      pen_(pen),
      on_change_(std::move(on_change)),
      smoothing_(new NumberField(this)),
      pressure_(new QCheckBox(QStringLiteral("Pressure sets width"), this)),
      close_ends_(new QCheckBox(QStringLiteral("Join ends that meet"), this)) {
  const PenSettings& settings = pen.Settings();
  smoothing_->setRange(0, 100);
  smoothing_->setDecimals(0);
  smoothing_->setSuffix(QStringLiteral(" %"));
  smoothing_->setValue(settings.smoothing * 100);
  smoothing_->setFixedWidth(kFieldWidth);
  smoothing_->setObjectName(QStringLiteral("pen-smoothing"));

  pressure_->setChecked(settings.pressure_width);
  close_ends_->setChecked(settings.close_ends);

  auto* form = new QFormLayout();
  form->setLabelAlignment(Qt::AlignLeft);
  form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
  form->addRow(QStringLiteral("Smoothing"), smoothing_);

  auto* layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addWidget(pressure_);
  layout->addWidget(close_ends_);
  layout->addStretch();

  connect(smoothing_, &NumberField::valueChanged, this, [this] { Apply(); });
  for (const QCheckBox* box : {pressure_, close_ends_}) {
    connect(box, &QCheckBox::toggled, this, [this] { Apply(); });
  }
}

void PenOptions::Apply() {
  const PenSettings settings{
      .smoothing = smoothing_->value() / 100,
      .pressure_width = pressure_->isChecked(),
      .close_ends = close_ends_->isChecked(),
  };
  pen_.SetSettings(settings);
  on_change_(pen_.Settings());
}

}  // namespace snapper
