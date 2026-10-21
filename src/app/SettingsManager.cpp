#include "app/SettingsManager.h"

#include <QSettings>

namespace snapper {

namespace {

const QString kLayoutKey = QStringLiteral("window/panel_layout");
const QString kLayoutVersionKey = QStringLiteral("window/panel_layout_version");
const QString kPenSmoothingKey = QStringLiteral("pen/smoothing");
const QString kPenWidthKey = QStringLiteral("pen/line_width");
const QString kPenPressureKey = QStringLiteral("pen/pressure_width");
const QString kPenCloseKey = QStringLiteral("pen/close_ends");
const QString kPenFillKey = QStringLiteral("pen/fill_closed");

}  // namespace

SettingsManager::SettingsManager() : store_(std::make_unique<QSettings>()) {}

SettingsManager::SettingsManager(std::unique_ptr<QSettings> store)
    : store_(std::move(store)) {}

SettingsManager::~SettingsManager() = default;

QByteArray SettingsManager::PanelLayout() const {
  const bool is_current =
      store_->value(kLayoutVersionKey).toInt() == kLayoutVersion;
  if (!is_current) {
    return {};
  }
  return store_->value(kLayoutKey).toByteArray();
}

void SettingsManager::SetPanelLayout(const QByteArray& layout) {
  store_->setValue(kLayoutKey, layout);
  store_->setValue(kLayoutVersionKey, kLayoutVersion);
  store_->sync();
}

PenSettings SettingsManager::Pen() const {
  const PenSettings defaults;
  return {
      .smoothing =
          store_->value(kPenSmoothingKey, defaults.smoothing).toDouble(),
      .line_width = store_->value(kPenWidthKey, defaults.line_width).toDouble(),
      .pressure_width =
          store_->value(kPenPressureKey, defaults.pressure_width).toBool(),
      .close_ends = store_->value(kPenCloseKey, defaults.close_ends).toBool(),
      .fill_closed = store_->value(kPenFillKey, defaults.fill_closed).toBool(),
  };
}

void SettingsManager::SetPen(const PenSettings& pen) {
  store_->setValue(kPenSmoothingKey, pen.smoothing);
  store_->setValue(kPenWidthKey, pen.line_width);
  store_->setValue(kPenPressureKey, pen.pressure_width);
  store_->setValue(kPenCloseKey, pen.close_ends);
  store_->setValue(kPenFillKey, pen.fill_closed);
}

}  // namespace snapper
