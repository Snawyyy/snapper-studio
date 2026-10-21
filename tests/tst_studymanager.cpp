#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTest>
#include <QWidget>

#include "app/CommandRegistry.h"
#include "app/StudyManager.h"
#include "edit/UndoManager.h"

using snapper::StudyManager;

namespace {

QList<QJsonObject> ReadSession(const QString& path) {
  QFile file(path);
  QList<QJsonObject> events;
  if (!file.open(QIODevice::ReadOnly)) {
    return events;
  }
  while (!file.atEnd()) {
    events.append(QJsonDocument::fromJson(file.readLine()).object());
  }
  return events;
}

}  // namespace

class StudyManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void initTestCase() { QStandardPaths::setTestModeEnabled(true); }

  void RecordsCommandsAndNotes() {
    QWidget window;
    snapper::CommandRegistry commands;
    snapper::UndoManager undo;
    StudyManager study(&window, commands, undo);
    commands.Add({.id = QStringLiteral("test.thing"),
                  .title = QStringLiteral("Thing"),
                  .shortcut = {},
                  .run = [] {}});
    QString error;
    QVERIFY2(study.Start(&error), qPrintable(error));
    commands.Run(QStringLiteral("test.thing"));
    study.RecordNote(QStringLiteral("finding the layers"), 3, QPoint(5, 6));
    const QString path = study.SessionPath();
    study.Stop();
    QVERIFY(!study.IsRecording());

    const QList<QJsonObject> events = ReadSession(path);
    QCOMPARE(events.size(), 4);
    QCOMPARE(events[0].value(QStringLiteral("type")).toString(),
             QStringLiteral("start"));
    QCOMPARE(events[1].value(QStringLiteral("id")).toString(),
             QStringLiteral("test.thing"));
    QCOMPARE(events[2].value(QStringLiteral("frustration")).toInt(), 3);
    QCOMPARE(events[3].value(QStringLiteral("type")).toString(),
             QStringLiteral("stop"));
    QFile::remove(path);
  }

  void NothingIsWrittenWhenOff() {
    QWidget window;
    snapper::CommandRegistry commands;
    snapper::UndoManager undo;
    const StudyManager study(&window, commands, undo);
    QVERIFY(!study.IsRecording());
    QVERIFY(study.SessionPath().isEmpty());
  }
};

QTEST_MAIN(StudyManagerTest)
#include "tst_studymanager.moc"
