#include <kddockwidgets/KDDockWidgets.h>

#include <QApplication>

#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("Snapper Studio"));
  QApplication::setOrganizationName(QStringLiteral("Snapper"));
  KDDockWidgets::initFrontend(KDDockWidgets::FrontendType::QtWidgets);

  snapper::MainWindow window;
  window.show();
  return QApplication::exec();
}
