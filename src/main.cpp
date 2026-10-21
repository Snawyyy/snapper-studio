#include <kddockwidgets/KDDockWidgets.h>

#include <QApplication>

#include "app/ThemeManager.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("Snapper Studio"));
  QApplication::setOrganizationName(QStringLiteral("Snapper"));
  // A bad theme file keeps the app usable with Qt's own look.
  snapper::ThemeManager theme;
  const bool is_theme_loaded =
      theme.LoadFromFile(QStringLiteral(":/theme/theme.json"));
  if (is_theme_loaded) {
    theme.ApplyTo(app);
  } else {
    qWarning("%s", qPrintable(theme.Error()));
  }

  KDDockWidgets::initFrontend(KDDockWidgets::FrontendType::QtWidgets);

  snapper::MainWindow window;
  window.show();
  return QApplication::exec();
}
