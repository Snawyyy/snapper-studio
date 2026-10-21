#include "app/Startup.h"

#include <kddockwidgets/Config.h>
#include <kddockwidgets/KDDockWidgets.h>

#include <QApplication>

#include "app/ThemeManager.h"

namespace snapper {

void SetUpApplication(QApplication& app) {
  QApplication::setApplicationName(QStringLiteral("Snapper Studio"));
  QApplication::setOrganizationName(QStringLiteral("Snapper"));

  // A bad theme file keeps the app usable with Qt's own look.
  ThemeManager theme;
  const bool is_theme_loaded =
      theme.LoadFromFile(QStringLiteral(":/theme/theme.json"));
  if (is_theme_loaded) {
    theme.ApplyTo(app);
  } else {
    qWarning("%s", qPrintable(theme.Error()));
  }

  KDDockWidgets::initFrontend(KDDockWidgets::FrontendType::QtWidgets);

  // A panel's close button closes only the tab showing, not its neighbors.
  KDDockWidgets::Config& docks = KDDockWidgets::Config::self();
  docks.setFlags(docks.flags() |
                 KDDockWidgets::Config::Flag_CloseOnlyCurrentTab);
}

}  // namespace snapper
