#include <QApplication>

#include "app/Startup.h"
#include "app/Studio.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  snapper::SetUpApplication(app);

  snapper::Studio studio;
  studio.Show();
  return QApplication::exec();
}
