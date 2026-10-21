#include "app/SettingsManager.h"

#include <QSettings>

namespace snapper {

namespace {

const QString kLayoutKey = QStringLiteral("window/panel_layout");
const QString kLayoutVersionKey = QStringLiteral("window/panel_layout_version");
const QString kPenSmoothingKey = QStringLiteral("pen/smoothing");
const QString kPenPressureKey = QStringLiteral("pen/pressure_width");
const QString kPenCloseKey = QStringLiteral("pen/close_ends");
const QString kLookHasFillKey = QStringLiteral("look/has_fill");
const QString kLookFillKey = QStringLiteral("look/fill");
const QString kLookHasLineKey = QStringLiteral("look/has_line");
const QString kLookLineKey = QStringLiteral("look/line");
const QString kLookWidthKey = QStringLiteral("look/line_width");

QColor ColorOr(const QVariant& value, const QColor& fallback) {
  const QColor color = QColor::fromString(value.toString());
  return color.isValid() ? color : fallback;
}

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
      .pressure_width =
          store_->value(kPenPressureKey, defaults.pressure_width).toBool(),
      .close_ends = store_->value(kPenCloseKey, defaults.close_ends).toBool(),
  };
}

void SettingsManager::SetPen(const PenSettings& pen) {
  store_->setValue(kPenSmoothingKey, pen.smoothing);
  store_->setValue(kPenPressureKey, pen.pressure_width);
  store_->setValue(kPenCloseKey, pen.close_ends);
}

ShapeLook SettingsManager::NewShapeLook() const {
  const ShapeLook defaults;
  ShapeLook look;
  look.has_fill = store_->value(kLookHasFillKey, defaults.has_fill).toBool();
  look.fill = ColorOr(store_->value(kLookFillKey), defaults.fill);
  look.has_line = store_->value(kLookHasLineKey, defaults.has_line).toBool();
  look.line = ColorOr(store_->value(kLookLineKey), defaults.line);
  look.line_width = store_->value(kLookWidthKey, defaults.line_width).toFloat();
  return look;
}

void SettingsManager::SetNewShapeLook(const ShapeLook& look) {
  store_->setValue(kLookHasFillKey, look.has_fill);
  store_->setValue(kLookFillKey, look.fill.name(QColor::HexArgb));
  store_->setValue(kLookHasLineKey, look.has_line);
  store_->setValue(kLookLineKey, look.line.name(QColor::HexArgb));
  store_->setValue(kLookWidthKey, look.line_width);
}

}  // namespace snapper
