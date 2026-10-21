#include "app/StudyManager.h"

#include <QAbstractButton>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QStandardPaths>
#include <QTabletEvent>
#include <QTextEdit>
#include <QUndoStack>

#include <map>
#include <utility>

#include "app/CommandRegistry.h"
#include "edit/UndoManager.h"

namespace snapper {

namespace {

QString ButtonName(Qt::MouseButton button) {
  switch (button) {
    case Qt::RightButton:
      return QStringLiteral("right");
    case Qt::MiddleButton:
      return QStringLiteral("middle");
    default:
      return QStringLiteral("left");
  }
}

bool IsModifierOnly(int key) {
  return key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt ||
         key == Qt::Key_Meta || key == Qt::Key_AltGr;
}

bool IsTextField(const QObject* object) {
  return qobject_cast<const QLineEdit*>(object) != nullptr ||
         qobject_cast<const QTextEdit*>(object) != nullptr;
}

}  // namespace

StudyManager::StudyManager(QWidget* window, CommandRegistry& commands,
                           UndoManager& undo)
    : window_(window) {
  connect(&commands, &CommandRegistry::Ran, this, [this](const QString& id) {
    if (IsRecording()) {
      Write({{QStringLiteral("type"), QStringLiteral("command")},
             {QStringLiteral("id"), id}});
    }
  });
  connect(undo.Stack(), &QUndoStack::indexChanged, this,
          [this, &undo](int index) {
            if (IsRecording()) {
              Write({{QStringLiteral("type"), QStringLiteral("history")},
                     {QStringLiteral("index"), index},
                     {QStringLiteral("undo_text"), undo.UndoText()}});
            }
          });
}

StudyManager::~StudyManager() { Stop(); }

bool StudyManager::Start(QString* error) {
  if (IsRecording()) {
    return true;
  }
  const QString folder =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      QStringLiteral("/studies");
  QDir().mkpath(folder);
  const QString name = QStringLiteral("session-%1.jsonl")
                           .arg(QDateTime::currentDateTime().toString(
                               QStringLiteral("yyyyMMdd-HHmmss")));
  file_.setFileName(folder + u'/' + name);
  if (!file_.open(QIODevice::WriteOnly | QIODevice::Append)) {
    *error = QStringLiteral("Can't write %1").arg(file_.fileName());
    return false;
  }
  clock_.start();
  last_move_ms_ = -kMoveEveryMs;
  recent_.clear();
  Write({{QStringLiteral("type"), QStringLiteral("start")},
         {QStringLiteral("app_version"), QStringLiteral(SNAPPER_VERSION)},
         {QStringLiteral("window"),
          QJsonArray{window_->width(), window_->height()}}});
  qApp->installEventFilter(this);
  Q_EMIT RecordingChanged(true);
  return true;
}

void StudyManager::Stop() {
  if (!IsRecording()) {
    return;
  }
  qApp->removeEventFilter(this);
  Write({{QStringLiteral("type"), QStringLiteral("stop")}});
  const QString path = file_.fileName();
  file_.close();
  Q_EMIT RecordingChanged(false);
  Q_EMIT Hint(QStringLiteral("Study session saved to %1").arg(path));
}

void StudyManager::RegisterCommands(CommandRegistry& commands) {
  commands.Add({
      .id = QStringLiteral("study.toggle"),
      .title = QStringLiteral("Start or Stop Study Recording"),
      .shortcut = {},
      .run =
          [this] {
            if (IsRecording()) {
              Stop();
              return;
            }
            QString error;
            if (Start(&error)) {
              Q_EMIT Hint(QStringLiteral(
                  "Recording a study session. Ctrl+Alt+N adds a note."));
            } else {
              Q_EMIT Hint(error);
            }
          },
  });
}

// Divides the last few seconds into cells and returns the middle of the
// cell the pointer spent the longest in. Each sample counts for the time
// until the next one.
QPoint StudyManager::RestSpot() const {
  if (recent_.empty()) {
    return window_->mapFromGlobal(QCursor::pos());
  }
  const qint64 now = clock_.elapsed();
  std::map<std::pair<int, int>, qint64> dwell;
  for (size_t i = 0; i < recent_.size(); ++i) {
    const qint64 until = i + 1 < recent_.size() ? recent_[i + 1].ms : now;
    const QPoint& pos = recent_[i].pos;
    dwell[{pos.x() / kRestCellPx, pos.y() / kRestCellPx}] +=
        until - recent_[i].ms;
  }
  auto best = dwell.cbegin();
  for (auto it = dwell.cbegin(); it != dwell.cend(); ++it) {
    if (it->second > best->second) {
      best = it;
    }
  }
  return {best->first.first * kRestCellPx + kRestCellPx / 2,
          best->first.second * kRestCellPx + kRestCellPx / 2};
}

void StudyManager::RecordNote(const QString& text, int frustration,
                              const QPoint& spot) {
  if (!IsRecording()) {
    return;
  }
  Write({{QStringLiteral("type"), QStringLiteral("note")},
         {QStringLiteral("text"), text},
         {QStringLiteral("frustration"), frustration},
         {QStringLiteral("x"), spot.x()},
         {QStringLiteral("y"), spot.y()}});
}

bool StudyManager::eventFilter(QObject* watched, QEvent* event) {
  // Events reach a widget and then its parents, and windows see them too.
  // Each one is written once, from the widget it first reached.
  const bool is_widget = watched->isWidgetType();
  switch (event->type()) {
    case QEvent::MouseMove:
    case QEvent::TabletMove: {
      if (!is_widget) {
        break;
      }
      const bool is_pen = event->type() == QEvent::TabletMove;
      const auto* pointer = static_cast<const QSinglePointEvent*>(event);
      RecordPointer(pointer->globalPosition().toPoint(), is_pen);
      break;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::TabletPress: {
      const auto* pointer = static_cast<const QSinglePointEvent*>(event);
      const bool is_new = is_widget && pointer->timestamp() != last_input_ms_;
      if (!is_new) {
        break;
      }
      last_input_ms_ = pointer->timestamp();
      const QPoint pos =
          window_->mapFromGlobal(pointer->globalPosition().toPoint());
      const bool is_double = event->type() == QEvent::MouseButtonDblClick;
      Write({{QStringLiteral("type"), is_double ? QStringLiteral("doubleclick")
                                                : QStringLiteral("click")},
             {QStringLiteral("button"), ButtonName(pointer->button())},
             {QStringLiteral("pen"), event->type() == QEvent::TabletPress},
             {QStringLiteral("x"), pos.x()},
             {QStringLiteral("y"), pos.y()},
             {QStringLiteral("control"), ControlName(watched)}});
      break;
    }
    case QEvent::KeyPress: {
      const auto* key = static_cast<const QKeyEvent*>(event);
      const bool is_new = is_widget && !key->isAutoRepeat() &&
                          !IsModifierOnly(key->key()) &&
                          key->timestamp() != last_key_ms_;
      if (!is_new) {
        break;
      }
      last_key_ms_ = key->timestamp();
      // In text fields only the fact of typing is kept, never the text.
      const bool is_typing = IsTextField(watched) && !key->text().isEmpty() &&
                             key->text().at(0).isPrint();
      if (is_typing) {
        Write({{QStringLiteral("type"), QStringLiteral("typing")}});
        break;
      }
      const QKeySequence sequence(key->keyCombination());
      Write({{QStringLiteral("type"), QStringLiteral("key")},
             {QStringLiteral("key"), sequence.toString()},
             {QStringLiteral("control"), ControlName(watched)}});
      break;
    }
    default:
      break;
  }
  return QObject::eventFilter(watched, event);
}

void StudyManager::Write(QJsonObject event) {
  event.insert(QStringLiteral("t"), clock_.elapsed());
  file_.write(QJsonDocument(event).toJson(QJsonDocument::Compact));
  file_.write("\n");
  file_.flush();
}

void StudyManager::RecordPointer(const QPoint& global, bool is_pen) {
  const qint64 now = clock_.elapsed();
  const bool is_due = now - last_move_ms_ >= kMoveEveryMs;
  if (!is_due) {
    return;
  }
  last_move_ms_ = now;
  const QPoint pos = window_->mapFromGlobal(global);
  recent_.push_back({.ms = now, .pos = pos});
  while (!recent_.empty() && now - recent_.front().ms > kRestWindowMs) {
    recent_.pop_front();
  }
  Write({{QStringLiteral("type"), QStringLiteral("move")},
         {QStringLiteral("pen"), is_pen},
         {QStringLiteral("x"), pos.x()},
         {QStringLiteral("y"), pos.y()}});
}

// The nearest named control and its kind, plus a button's label, like
// QPushButton#layer-add "New Layer".
QString StudyManager::ControlName(const QObject* object) {
  QString name = QString::fromLatin1(object->metaObject()->className());
  for (const QObject* at = object; at != nullptr; at = at->parent()) {
    if (!at->objectName().isEmpty()) {
      name += u'#' + at->objectName();
      break;
    }
  }
  const auto* button = qobject_cast<const QAbstractButton*>(object);
  const bool has_label = button != nullptr && !button->text().isEmpty();
  if (has_label) {
    name += QStringLiteral(" \"%1\"").arg(button->text());
  }
  return name;
}

}  // namespace snapper
