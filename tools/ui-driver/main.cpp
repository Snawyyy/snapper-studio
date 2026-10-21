// snapper-drive: runs Snapper Studio offscreen and follows a script of UI
// actions, so changes can be checked without touching anyone's screen.
//
//   snapper-drive script.txt      read the script from a file
//   snapper-drive -               read it from stdin
//
// One action per line, # starts a comment. Coordinates are window pixels.
//   size W H                 resize the window
//   wait MS                  let the app run for a while
//   click X Y [right]        click a point
//   doubleclick X Y
//   drag X1 Y1 X2 Y2         press, move in steps, release
//   hover X Y                move the pointer
//   key SEQUENCE             press keys, like Ctrl+K or Escape
//   type TEXT                type text into the focused control
//   shot PATH                save a screenshot of the window, with
//                            floating panels and popups drawn on top
//   focus                    print which control has focus
//   windows                  print every visible window and where it is
//   quit                     stop (also happens at the end of the script)

#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QStandardPaths>
#include <QTest>
#include <QTextStream>
#include <QTimer>
#include <QWindow>

#include <cstdio>
#include <vector>

#include "app/Startup.h"
#include "app/Studio.h"

namespace {

constexpr int kStepDelayMs = 50;
constexpr int kDragSteps = 12;
constexpr int kMaxScriptLines = 10000;

struct Step {
  int line = 0;
  QStringList words;
  QString rest;  // everything after the first word, for "type"
};

bool ReadScript(const QString& path, std::vector<Step>& steps) {
  QFile file(path);
  const bool is_stdin = path == QStringLiteral("-");
  const bool is_open = is_stdin ? file.open(stdin, QIODevice::ReadOnly)
                                : file.open(QIODevice::ReadOnly);
  if (!is_open) {
    std::fprintf(stderr, "can't open script %s\n", qPrintable(path));
    return false;
  }

  QTextStream in(&file);
  int line_number = 0;
  while (!in.atEnd() && line_number < kMaxScriptLines) {
    ++line_number;
    const QString line = in.readLine().trimmed();
    const bool is_blank = line.isEmpty() || line.startsWith(u'#');
    if (is_blank) {
      continue;
    }
    Step step;
    step.line = line_number;
    step.words = line.split(u' ', Qt::SkipEmptyParts);
    step.rest = line.mid(step.words.first().size()).trimmed();
    steps.push_back(step);
  }
  return true;
}

// Runs the script one step per event loop turn, so the app gets to react
// (repaint, open popups, run timers) between actions.
class Driver : public QObject {
 public:
  Driver(QWidget* window, std::vector<Step> steps)
      : window_(window), steps_(std::move(steps)) {}

  void Start() { Schedule(kStepDelayMs); }
  int ExitCode() const { return exit_code_; }

 private:
  void Schedule(int delay_ms) {
    QTimer::singleShot(delay_ms, this, [this] { RunNext(); });
  }

  void Fail(const Step& step, const char* why) {
    std::fprintf(stderr, "line %d: %s\n", step.line, why);
    exit_code_ = 1;
    QApplication::quit();
  }

  QWindow* Target() const {
    QWindow* focus = QGuiApplication::focusWindow();
    const bool has_focus_window = focus != nullptr;
    return has_focus_window ? focus : window_->windowHandle();
  }

  bool ReadInts(const Step& step, int count, std::vector<int>& out) {
    if (step.words.size() < count + 1) {
      return false;
    }
    for (int i = 1; i <= count; ++i) {
      bool ok = false;
      out.push_back(step.words[i].toInt(&ok));
      if (!ok) {
        return false;
      }
    }
    return true;
  }

  void RunNext() {
    const bool is_done = next_ >= steps_.size();
    if (is_done) {
      Quit();
      return;
    }
    const Step& step = steps_[next_++];
    const QString& action = step.words.first();
    int delay_ms = kStepDelayMs;
    std::vector<int> n;

    if (action == u"size" && ReadInts(step, 2, n)) {
      window_->resize(n[0], n[1]);
    } else if (action == u"wait" && ReadInts(step, 1, n)) {
      delay_ms = n[0];
    } else if (action == u"click" && ReadInts(step, 2, n)) {
      const bool is_right = step.words.size() > 3 && step.words[3] == u"right";
      const Qt::MouseButton button =
          is_right ? Qt::RightButton : Qt::LeftButton;
      QTest::mouseClick(Target(), button, {}, QPoint(n[0], n[1]));
    } else if (action == u"doubleclick" && ReadInts(step, 2, n)) {
      QTest::mouseDClick(Target(), Qt::LeftButton, {}, QPoint(n[0], n[1]));
    } else if (action == u"hover" && ReadInts(step, 2, n)) {
      QTest::mouseMove(Target(), QPoint(n[0], n[1]));
    } else if (action == u"drag" && ReadInts(step, 4, n)) {
      Drag(QPoint(n[0], n[1]), QPoint(n[2], n[3]));
    } else if (action == u"key" && step.words.size() > 1) {
      QTest::keySequence(Target(), QKeySequence(step.words[1]));
    } else if (action == u"type" && !step.rest.isEmpty()) {
      QTest::keyClicks(QApplication::focusWidget(), step.rest);
    } else if (action == u"shot" && step.words.size() > 1) {
      if (!Screenshot().save(step.rest)) {
        Fail(step, "can't save screenshot");
        return;
      }
      std::printf("saved %s\n", qPrintable(step.rest));
    } else if (action == u"focus") {
      PrintFocus();
    } else if (action == u"windows") {
      PrintWindows();
    } else if (action == u"quit") {
      Quit();
      return;
    } else {
      Fail(step, "unknown action or bad arguments");
      return;
    }
    Schedule(delay_ms);
  }

  // Closes the main window like a user would, so the layout gets saved,
  // then makes sure the app stops.
  void Quit() {
    window_->close();
    QApplication::quit();
  }

  void Drag(QPoint from, QPoint to) {
    QWindow* target = Target();
    QTest::mouseMove(target, from);
    QTest::mousePress(target, Qt::LeftButton, {}, from);
    for (int i = 1; i <= kDragSteps; ++i) {
      const QPoint point = from + (to - from) * i / kDragSteps;
      QTest::mouseMove(target, point);
    }
    QTest::mouseRelease(target, Qt::LeftButton, {}, to);
  }

  // The main window with every other visible window (floating panels,
  // popups) painted on top where it sits.
  QPixmap Screenshot() const {
    QPixmap shot = window_->grab();
    QPainter painter(&shot);
    const QWidgetList windows = QApplication::topLevelWidgets();
    for (QWidget* other : windows) {
      const bool is_overlay = other != window_ && other->isVisible();
      if (is_overlay) {
        painter.drawPixmap(other->pos() - window_->pos(), other->grab());
      }
    }
    return shot;
  }

  void PrintWindows() const {
    const QWidgetList windows = QApplication::topLevelWidgets();
    for (const QWidget* window : windows) {
      if (!window->isVisible()) {
        continue;
      }
      const QRect box = window->geometry();
      std::printf("window: %s \"%s\" at %d,%d size %dx%d\n",
                  window->metaObject()->className(),
                  qPrintable(window->windowTitle()), box.x(), box.y(),
                  box.width(), box.height());
    }
  }

  void PrintFocus() const {
    const QWidget* focus = QApplication::focusWidget();
    if (focus == nullptr) {
      std::printf("focus: none\n");
      return;
    }
    std::printf("focus: %s \"%s\"\n", focus->metaObject()->className(),
                qPrintable(focus->objectName()));
  }

  QWidget* window_;
  std::vector<Step> steps_;
  size_t next_ = 0;
  int exit_code_ = 0;
};

}  // namespace

int main(int argc, char* argv[]) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);
  if (argc < 2) {
    std::fprintf(stderr, "usage: snapper-drive SCRIPT | -\n");
    return 2;
  }

  // Keeps the driver's remembered settings away from the real ones.
  QStandardPaths::setTestModeEnabled(true);
  snapper::SetUpApplication(app);

  std::vector<Step> steps;
  if (!ReadScript(QString::fromLocal8Bit(argv[1]), steps)) {
    return 2;
  }

  snapper::Studio studio;
  studio.Show();
  Driver driver(studio.Window(), std::move(steps));
  driver.Start();
  const int status = QApplication::exec();
  return status != 0 ? status : driver.ExitCode();
}
