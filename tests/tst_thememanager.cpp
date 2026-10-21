#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "app/ThemeManager.h"

namespace {

QByteArray ShippedTheme() {
  QFile file(QStringLiteral(SNAPPER_THEME_FILE));
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  return file.readAll();
}

}  // namespace

class ThemeManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void LoadsShippedTheme() {
    snapper::ThemeManager themes;
    QVERIFY2(themes.LoadFromJson(ShippedTheme()), qPrintable(themes.Error()));
    QCOMPARE(themes.CurrentTheme().accent, QColor(0xD6, 0x3D, 0x55));
    QCOMPARE(themes.CurrentTheme().text.alpha(), 0xD9);
    QCOMPARE(themes.CurrentTheme().row_height, 28);
  }

  void RejectsBrokenJson() {
    snapper::ThemeManager themes;
    QVERIFY(!themes.LoadFromJson("{ not json"));
    QVERIFY(!themes.Error().isEmpty());
  }

  void NamesTheMissingColor() {
    QJsonObject root = QJsonDocument::fromJson(ShippedTheme()).object();
    QJsonObject colors = root.value(QStringLiteral("colors")).toObject();
    colors.remove(QStringLiteral("accent"));
    root.insert(QStringLiteral("colors"), colors);

    snapper::ThemeManager themes;
    QVERIFY(!themes.LoadFromJson(QJsonDocument(root).toJson()));
    QVERIFY(themes.Error().contains(QStringLiteral("accent")));
  }

  void KeepsOldThemeAfterFailure() {
    snapper::ThemeManager themes;
    QVERIFY(themes.LoadFromJson(ShippedTheme()));
    QVERIFY(!themes.LoadFromJson("[]"));
    QCOMPARE(themes.CurrentTheme().accent, QColor(0xD6, 0x3D, 0x55));
  }

  void StyleSheetUsesThemeColors() {
    snapper::ThemeManager themes;
    QVERIFY(themes.LoadFromJson(ShippedTheme()));
    QVERIFY(themes.BuildStyleSheet().contains(
        QStringLiteral("rgba(214, 61, 85, 255)")));
  }
};

QTEST_GUILESS_MAIN(ThemeManagerTest)
#include "tst_thememanager.moc"
