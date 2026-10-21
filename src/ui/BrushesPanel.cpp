#include "ui/BrushesPanel.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "document/Document.h"
#include "edit/BrushActions.h"
#include "ui/BrushPreview.h"
#include "ui/NumberField.h"

namespace snapper {

namespace {

constexpr QSize kListSample(96, 24);
constexpr QSize kEditorSample(200, 48);
constexpr int kListHeight = 168;
constexpr int kFieldWidth = 72;

NumberField* Field(double max, int decimals, const QString& suffix,
                   QWidget* parent) {
  auto* field = new NumberField(parent);
  field->setRange(0, max);
  field->setDecimals(decimals);
  field->setSuffix(suffix);
  field->setFixedWidth(kFieldWidth);
  return field;
}

}  // namespace

BrushesPanel::BrushesPanel(Document& document, BrushActions& actions,
                           const Theme& theme, QWidget* parent)
    : QWidget(parent),
      document_(document),
      actions_(actions),
      theme_(theme),
      list_(new QListWidget()),
      import_(new QPushButton(QStringLiteral("Import"))),
      use_(new QPushButton(QStringLiteral("Use"))),
      delete_(new QPushButton(QStringLiteral("Delete"))),
      editor_(new QWidget()),
      name_(new QLineEdit(editor_)),
      sample_(new QLabel(editor_)),
      spacing_(
          Field(Brush::kMaxSpacing * 100, 0, QStringLiteral(" %"), editor_)),
      align_(new QCheckBox(QStringLiteral("Turn along the line"), editor_)),
      angle_jitter_(
          Field(Brush::kMaxAngleJitter, 0, QStringLiteral(" deg"), editor_)),
      spacing_jitter_(Field(100, 0, QStringLiteral(" %"), editor_)),
      size_jitter_(Field(100, 0, QStringLiteral(" %"), editor_)),
      boil_every_(Field(Brush::kMaxBoilEvery, 0, QString(), editor_)) {
  list_->setObjectName(QStringLiteral("brush-list"));
  list_->setIconSize(kListSample);
  list_->setFixedHeight(kListHeight);
  import_->setObjectName(QStringLiteral("brush-import"));
  import_->setToolTip(QStringLiteral(
      "Add a picture as a brush. Its see-through parts are left bare, or its "
      "light parts if it has no transparency."));
  use_->setObjectName(QStringLiteral("brush-use"));
  use_->setToolTip(
      QStringLiteral("Draw the selected shapes, or new shapes, with it"));
  delete_->setObjectName(QStringLiteral("brush-delete"));
  name_->setObjectName(QStringLiteral("brush-name"));
  name_->setPlaceholderText(QStringLiteral("Brush name"));
  spacing_->setObjectName(QStringLiteral("brush-spacing"));
  spacing_->setMinimum(Brush::kMinSpacing * 100);
  spacing_->setToolTip(
      QStringLiteral("Gap between dabs, as a share of the line width"));
  boil_every_->setObjectName(QStringLiteral("brush-boil"));
  boil_every_->setSpecialValueText(QStringLiteral("Never"));
  boil_every_->setToolTip(QStringLiteral(
      "Frames between new random dabs, so the line shimmers when played"));

  auto* buttons = new QHBoxLayout();
  buttons->addWidget(import_);
  buttons->addWidget(use_);
  buttons->addWidget(delete_);
  buttons->addStretch();

  auto* grid = new QGridLayout();
  grid->setHorizontalSpacing(8);
  grid->setVerticalSpacing(6);
  const auto row = [&](const QString& label, QWidget* field, int at) {
    grid->addWidget(new QLabel(label, editor_), at, 0);
    grid->addWidget(field, at, 1);
  };
  row(QStringLiteral("Spacing"), spacing_, 0);
  grid->addWidget(align_, 1, 0, 1, 2);
  row(QStringLiteral("Turn jitter"), angle_jitter_, 2);
  row(QStringLiteral("Gap jitter"), spacing_jitter_, 3);
  row(QStringLiteral("Size jitter"), size_jitter_, 4);
  row(QStringLiteral("Boil every"), boil_every_, 5);
  auto* frames = new QLabel(QStringLiteral("frames"), editor_);
  frames->setObjectName(QStringLiteral("secondary"));
  grid->addWidget(frames, 5, 2);
  grid->setColumnStretch(3, 1);

  auto* editor_layout = new QVBoxLayout(editor_);
  editor_layout->setContentsMargins(0, 0, 0, 0);
  editor_layout->addWidget(name_);
  editor_layout->addWidget(sample_);
  editor_layout->addLayout(grid);

  auto* content = new QWidget();
  auto* content_layout = new QVBoxLayout(content);
  content_layout->setContentsMargins(0, 0, 0, 0);
  content_layout->addWidget(list_);
  content_layout->addLayout(buttons);
  content_layout->addSpacing(8);
  content_layout->addWidget(editor_);
  content_layout->addStretch();

  auto* scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll->setWidget(content);
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(scroll);

  Bind(spacing_, QStringLiteral("Spacing"), [](Brush& brush, double value) {
    brush.spacing = static_cast<float>(value / 100);
  });
  Bind(angle_jitter_, QStringLiteral("Turn Jitter"),
       [](Brush& brush, double value) {
         brush.angle_jitter = static_cast<float>(value);
       });
  Bind(spacing_jitter_, QStringLiteral("Gap Jitter"),
       [](Brush& brush, double value) {
         brush.spacing_jitter = static_cast<float>(value / 100);
       });
  Bind(size_jitter_, QStringLiteral("Size Jitter"),
       [](Brush& brush, double value) {
         brush.size_jitter = static_cast<float>(value / 100);
       });
  Bind(boil_every_, QStringLiteral("Boil"), [](Brush& brush, double value) {
    brush.boil_every = static_cast<int>(value);
  });
  connect(align_, &QCheckBox::toggled, this, [this](bool align) {
    actions_.EditBrush(CurrentBrush(),
                       [align](Brush& brush) { brush.align = align; });
  });
  connect(name_, &QLineEdit::editingFinished, this,
          [this] { actions_.RenameBrush(CurrentBrush(), name_->text()); });
  connect(import_, &QPushButton::clicked, this, &BrushesPanel::ImportBrush);
  connect(use_, &QPushButton::clicked, this,
          [this] { actions_.UseBrush(CurrentBrush()); });
  connect(delete_, &QPushButton::clicked, this,
          [this] { actions_.DeleteBrush(CurrentBrush()); });
  connect(list_, &QListWidget::currentItemChanged, this,
          [this] { ShowBrush(); });
  const auto refill = [this] { Refill(); };
  connect(&document_.Brushes(), &BrushManager::ListChanged, this, refill);
  connect(&document_.Brushes(), &BrushManager::BrushChanged, this, refill);
  connect(&document_, &Document::Reset, this, refill);
  Refill();
}

void BrushesPanel::Bind(NumberField* field, const QString& what,
                        const std::function<void(Brush&, double)>& set) {
  drags_.push_back(std::make_unique<DragTagger>());
  DragTagger* drag = drags_.back().get();
  connect(field, &NumberField::valueChanged, this,
          [this, set, drag](double value) {
            actions_.EditBrush(
                CurrentBrush(), [&](Brush& brush) { set(brush, value); },
                drag->Tag());
          });
  field->setToolTip(field->toolTip().isEmpty() ? what : field->toolTip());
}

void BrushesPanel::ImportBrush() {
  const QString path = QFileDialog::getOpenFileName(
      window(), QStringLiteral("Import Brush"), QString(),
      QStringLiteral("Pictures (*.png *.jpg *.jpeg *.webp *.bmp)"));
  if (path.isEmpty()) {
    return;
  }
  QString why_not;
  const Id brush = actions_.ImportBrush(path, &why_not);
  if (brush == kNoId) {
    QMessageBox::warning(window(), QStringLiteral("Couldn't import the brush"),
                         why_not);
    return;
  }
  for (int i = 0; i < list_->count(); ++i) {
    if (list_->item(i)->data(Qt::UserRole).toInt() == brush) {
      list_->setCurrentRow(i);
    }
  }
}

Id BrushesPanel::CurrentBrush() const {
  const QListWidgetItem* item = list_->currentItem();
  return item == nullptr ? kNoId : item->data(Qt::UserRole).toInt();
}

// Rows keep their places: the list is rebuilt in the same order and only
// the highlight moves.
void BrushesPanel::Refill() {
  const Id current = CurrentBrush();
  const int old_row = list_->currentRow();
  {
    const QSignalBlocker blocker(list_);
    list_->clear();
    for (const Brush& brush : document_.Brushes().All()) {
      const QImage sample = BrushPreview(document_, brush.id, kListSample,
                                         theme_.text, devicePixelRatioF());
      auto* item = new QListWidgetItem(QIcon(QPixmap::fromImage(sample)),
                                       brush.name, list_);
      item->setData(Qt::UserRole, brush.id);
      if (brush.id == current) {
        list_->setCurrentItem(item);
      }
    }
    const bool is_lost = list_->currentItem() == nullptr && list_->count() > 0;
    if (is_lost) {
      list_->setCurrentRow(std::clamp(old_row, 0, list_->count() - 1));
    }
  }
  ShowBrush();
}

void BrushesPanel::ShowBrush() {
  const Brush* brush = document_.Brushes().Find(CurrentBrush());
  editor_->setEnabled(brush != nullptr);
  use_->setEnabled(brush != nullptr);
  delete_->setEnabled(brush != nullptr);
  const QString pick = QStringLiteral("pick a brush first");
  use_->setToolTip(brush != nullptr
                       ? QStringLiteral("Draw the selected shapes, or new "
                                        "shapes, with it")
                       : QStringLiteral("Use Brush: %1").arg(pick));
  delete_->setToolTip(brush != nullptr
                          ? actions_.DeleteEffect(brush->id)
                          : QStringLiteral("Delete Brush: %1").arg(pick));
  const Brush shown = brush != nullptr ? *brush : Brush{};
  const std::initializer_list<QWidget*> fields = {
      name_,           spacing_,     align_,     angle_jitter_,
      spacing_jitter_, size_jitter_, boil_every_};
  for (QWidget* field : fields) {
    field->blockSignals(true);
  }
  if (!name_->hasFocus()) {
    name_->setText(shown.name);
  }
  spacing_->setValue(shown.spacing * 100.0);
  align_->setChecked(shown.align);
  angle_jitter_->setValue(shown.angle_jitter);
  spacing_jitter_->setValue(shown.spacing_jitter * 100.0);
  size_jitter_->setValue(shown.size_jitter * 100.0);
  boil_every_->setValue(shown.boil_every);
  for (QWidget* field : fields) {
    field->blockSignals(false);
  }
  sample_->setPixmap(QPixmap::fromImage(BrushPreview(
      document_, shown.id, kEditorSample, theme_.text, devicePixelRatioF())));
}

}  // namespace snapper
