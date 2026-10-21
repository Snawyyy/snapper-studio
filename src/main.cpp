#include <QApplication>
#include <QMainWindow>

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("Snapper Studio"));
  QApplication::setOrganizationName(QStringLiteral("Snapper"));

  QMainWindow window;
  window.setWindowTitle(QStringLiteral("Snapper Studio"));
  window.resize(1280, 800);
  window.show();
  return QApplication::exec();
}
