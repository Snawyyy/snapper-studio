#include "app/SettingsManager.h"

#include <QSettings>

namespace snapper {

namespace {

const QString kLayoutKey = QStringLiteral("window/panel_layout");
const QString kLayoutVersionKey = QStringLiteral("window/panel_layout_version");

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

}  // namespace snapper
