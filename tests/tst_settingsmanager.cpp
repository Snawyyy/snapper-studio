#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "app/SettingsManager.h"

namespace {

std::unique_ptr<QSettings> StoreIn(const QTemporaryDir& dir) {
  return std::make_unique<QSettings>(dir.filePath(QStringLiteral("s.ini")),
                                     QSettings::IniFormat);
}

}  // namespace

class SettingsManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void NothingSavedIsEmpty() {
    const QTemporaryDir dir;
    const snapper::SettingsManager settings(StoreIn(dir));
    QVERIFY(settings.PanelLayout().isEmpty());
  }

  void LayoutSurvivesRestart() {
    const QTemporaryDir dir;
    {
      snapper::SettingsManager settings(StoreIn(dir));
      settings.SetPanelLayout("layout bytes");
    }
    const snapper::SettingsManager settings(StoreIn(dir));
    QCOMPARE(settings.PanelLayout(), QByteArray("layout bytes"));
  }

  void OldLayoutVersionIsDropped() {
    const QTemporaryDir dir;
    {
      std::unique_ptr<QSettings> store = StoreIn(dir);
      store->setValue(QStringLiteral("window/panel_layout"), QByteArray("old"));
      store->setValue(QStringLiteral("window/panel_layout_version"),
                      snapper::SettingsManager::kLayoutVersion - 1);
    }
    const snapper::SettingsManager settings(StoreIn(dir));
    QVERIFY(settings.PanelLayout().isEmpty());
  }
};

QTEST_GUILESS_MAIN(SettingsManagerTest)
#include "tst_settingsmanager.moc"
