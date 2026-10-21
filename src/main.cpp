#include <QApplication>

#include "app/Startup.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  snapper::SetUpApplication(app);

  snapper::MainWindow window;
  window.Show();
  return QApplication::exec();
}
