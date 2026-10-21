#pragma once

#include <QElapsedTimer>
#include <QFile>
#include <QJsonObject>
#include <QObject>
#include <QPoint>

#include <deque>

class QWidget;

namespace snapper {

class CommandRegistry;
class UndoManager;

// Usability recording, off until the user turns it on. While on, it writes
// one local file per session, one JSON line per event: pointer movement
// (thinned out), clicks with the name of the control hit, keys, commands
// run, undo steps, and notes. It never records artwork or typed text, and
// never uploads anything.
class StudyManager : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMoveEveryMs = 50;
  // A note pins to where the pointer rested longest in this time.
  static constexpr int kRestWindowMs = 3000;
  static constexpr int kRestCellPx = 24;

  StudyManager(QWidget* window, CommandRegistry& commands, UndoManager& undo);
  ~StudyManager() override;

  // Returns false, with the reason, when the session file can't be made.
  bool Start(QString* error);
  void Stop();
  bool IsRecording() const { return file_.isOpen(); }
  QString SessionPath() const { return file_.fileName(); }

  // Where the pointer rested longest lately, in window coordinates.
  QPoint RestSpot() const;
  void RecordNote(const QString& text, int frustration, const QPoint& spot);

  // Start or Stop Study Recording. The note command needs the note box
  // and is added by the studio.
  void RegisterCommands(CommandRegistry& commands);

 Q_SIGNALS:
  void RecordingChanged(bool recording);
  void Hint(const QString& text);

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  struct Rest {
    qint64 ms = 0;
    QPoint pos;
  };

  void Write(QJsonObject event);
  void RecordPointer(const QPoint& global, bool is_pen);
  static QString ControlName(const QObject* object);

  QWidget* window_;
  QFile file_;
  QElapsedTimer clock_;
  qint64 last_move_ms_ = -kMoveEveryMs;
  // Timestamps of the last press and key written, so an event passed on
  // to parent widgets is written once.
  quint64 last_input_ms_ = 0;
  quint64 last_key_ms_ = 0;
  std::deque<Rest> recent_;
};

}  // namespace snapper
