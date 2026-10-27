#include "ui/TimelinePanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "document/Document.h"
#include "edit/KeyActions.h"
#include "edit/Playhead.h"
#include "eval/Curves.h"
#include "ui/CanvasView.h"
#include "ui/NumberField.h"
#include "ui/TimelineView.h"

namespace snapper {

namespace {

NumberField* WholeNumber(QWidget* parent, const QString& name, int low,
                         int high) {
  auto* field = new NumberField(parent);
  field->setObjectName(name);
  field->setDecimals(0);
  field->setRange(low, high);
  return field;
}

}  // namespace

TimelinePanel::TimelinePanel(const Document& document, Playhead& playhead,
                             KeyActions& keys, CanvasView& canvas,
                             const Theme& theme, QWidget* parent)
    : QWidget(parent),
      playhead_(playhead),
      keys_(keys),
      canvas_(canvas),
      play_(new QPushButton(QStringLiteral("Play"), this)),
      frame_(WholeNumber(this, QStringLiteral("timeline-frame"), 0,
                         playhead.LastFrame())),
      curve_(new QComboBox(this)),
      onion_(new QCheckBox(QStringLiteral("Onion skin"), this)),
      onion_before_(WholeNumber(this, QStringLiteral("onion-before"), 0,
                                OnionSettings::kMaxFrames)),
      onion_after_(WholeNumber(this, QStringLiteral("onion-after"), 0,
                               OnionSettings::kMaxFrames)),
      view_(new TimelineView(document, playhead, keys, theme, this)) {
  play_->setObjectName(QStringLiteral("timeline-play"));
  play_->setToolTip(QStringLiteral("Play or Stop (Enter)"));
  frame_->setToolTip(QStringLiteral("The current frame. 0 is the setup pose."));
  curve_->setObjectName(QStringLiteral("timeline-curve"));
  curve_->setPlaceholderText(QStringLiteral("No keys selected"));
  for (int i = 0; i < kCurveCount; ++i) {
    curve_->addItem(Curves::Name(static_cast<Curve>(i)), i);
  }
  onion_->setObjectName(QStringLiteral("onion-skin"));
  onion_->setToolTip(QStringLiteral("Onion Skin (O)"));
  onion_before_->setToolTip(QStringLiteral("Frames shown before, in red"));
  onion_after_->setToolTip(QStringLiteral("Frames shown after, in green"));

  auto* bar = new QHBoxLayout();
  bar->addWidget(play_);
  bar->addWidget(new QLabel(QStringLiteral("Frame"), this));
  bar->addWidget(frame_);
  bar->addSpacing(12);
  bar->addWidget(new QLabel(QStringLiteral("Curve"), this));
  bar->addWidget(curve_);
  bar->addSpacing(12);
  bar->addWidget(onion_);
  bar->addWidget(onion_before_);
  bar->addWidget(onion_after_);
  bar->addStretch();

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(bar);
  layout->addWidget(view_, 1);

  connect(play_, &QPushButton::clicked, this,
          [this] { playhead_.TogglePlay(); });
  connect(&playhead_, &Playhead::PlayingChanged, this, [this](bool playing) {
    play_->setText(playing ? QStringLiteral("Stop") : QStringLiteral("Play"));
  });
  connect(frame_, &NumberField::valueChanged, this, [this](double frame) {
    playhead_.SetFrame(static_cast<int>(frame));
  });
  connect(&playhead_, &Playhead::FrameChanged, this,
          [this] { RefreshFrame(); });
  connect(&document, &Document::CanvasChanged, this,
          [this] { RefreshFrame(); });
  connect(curve_, &QComboBox::activated, this, [this](int index) {
    keys_.SetCurve(static_cast<Curve>(curve_->itemData(index).toInt()));
  });
  connect(&keys_, &KeyActions::KeysChanged, this, [this] { RefreshCurve(); });

  const auto onion_edited = [this] {
    OnionSettings onion;
    onion.on = onion_->isChecked();
    onion.before = static_cast<int>(onion_before_->value());
    onion.after = static_cast<int>(onion_after_->value());
    canvas_.SetOnion(onion);
  };
  connect(onion_, &QCheckBox::toggled, this, onion_edited);
  connect(onion_before_, &NumberField::valueChanged, this, onion_edited);
  connect(onion_after_, &NumberField::valueChanged, this, onion_edited);
  connect(&canvas_, &CanvasView::OnionChanged, this,
          [this] { RefreshOnion(); });

  RefreshFrame();
  RefreshCurve();
  RefreshOnion();
}

void TimelinePanel::RefreshFrame() {
  const QSignalBlocker blocker(frame_);
  frame_->setMaximum(playhead_.LastFrame());
  frame_->setValue(playhead_.Frame());
}

// Greyed out until keys are picked, and says so.
void TimelinePanel::RefreshCurve() {
  const bool has_keys = !keys_.Selected().empty();
  curve_->setEnabled(has_keys);
  curve_->setToolTip(has_keys
                         ? QStringLiteral("How the selected keys move on")
                         : QStringLiteral("Select keys to set their curve"));
  const std::optional<Curve> curve = keys_.SelectedCurve();
  const QSignalBlocker blocker(curve_);
  curve_->setCurrentIndex(curve.has_value() ? static_cast<int>(*curve) : -1);
}

void TimelinePanel::RefreshOnion() {
  const OnionSettings& onion = canvas_.Onion();
  const QSignalBlocker block_on(onion_);
  const QSignalBlocker block_before(onion_before_);
  const QSignalBlocker block_after(onion_after_);
  onion_->setChecked(onion.on);
  onion_before_->setValue(onion.before);
  onion_after_->setValue(onion.after);
  onion_before_->setEnabled(onion.on);
  onion_after_->setEnabled(onion.on);
}

}  // namespace snapper
