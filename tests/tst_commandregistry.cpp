#include <QSignalSpy>
#include <QTest>

#include "app/CommandRegistry.h"

namespace {

snapper::Command MakeCommand(const char* id, const char* title,
                             int* runs = nullptr) {
  return {
      .id = QString::fromLatin1(id),
      .title = QString::fromLatin1(title),
      .shortcut = {},
      .run =
          [runs] {
            if (runs != nullptr) {
              ++*runs;
            }
          },
  };
}

}  // namespace

class CommandRegistryTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void RunsAndAnnounces() {
    snapper::CommandRegistry commands;
    int runs = 0;
    QVERIFY(commands.Add(MakeCommand("a.run", "Run It", &runs)));
    QSignalSpy ran(&commands, &snapper::CommandRegistry::Ran);

    QVERIFY(commands.Run(QStringLiteral("a.run")));
    QCOMPARE(runs, 1);
    QCOMPARE(ran.count(), 1);
    QCOMPARE(ran.first().first().toString(), QStringLiteral("a.run"));
  }

  void RejectsBadCommands() {
    snapper::CommandRegistry commands;
    QVERIFY(commands.Add(MakeCommand("a", "A")));
    QVERIFY(!commands.Add(MakeCommand("a", "Again")));
    QVERIFY(!commands.Add(MakeCommand("", "No Id")));

    snapper::Command no_run = MakeCommand("b", "B");
    no_run.run = nullptr;
    QVERIFY(!commands.Add(no_run));
    QCOMPARE(commands.Count(), 1);
  }

  void StopsAtTheLimit() {
    snapper::CommandRegistry commands;
    for (int i = 0; i < snapper::CommandRegistry::kMaxCommands; ++i) {
      const QByteArray id = QByteArray::number(i);
      QVERIFY(commands.Add(MakeCommand(id.constData(), "Same")));
    }
    QVERIFY(!commands.Add(MakeCommand("over", "Over")));
  }

  void UnknownIdDoesNothing() {
    snapper::CommandRegistry commands;
    QVERIFY(!commands.Run(QStringLiteral("missing")));
  }

  void SearchRanksBestMatchFirst() {
    snapper::CommandRegistry commands;
    QVERIFY(commands.Add(MakeCommand("anywhere", "Relayout")));
    QVERIFY(commands.Add(MakeCommand("word", "Show Layers")));
    QVERIFY(commands.Add(MakeCommand("start", "Layer Options")));
    QVERIFY(commands.Add(MakeCommand("none", "Quit")));

    const std::vector<QString> ids = commands.Search(QStringLiteral("lay"));
    const std::vector<QString> expected = {QStringLiteral("start"),
                                           QStringLiteral("word"),
                                           QStringLiteral("anywhere")};
    QCOMPARE(ids, expected);
  }

  void SearchMatchesLettersInOrder() {
    snapper::CommandRegistry commands;
    QVERIFY(commands.Add(MakeCommand("layers", "Show Layers")));
    QVERIFY(commands.Add(MakeCommand("quit", "Quit")));

    const std::vector<QString> ids = commands.Search(QStringLiteral("sh ly"));
    QCOMPARE(ids.size(), size_t{1});
    QCOMPARE(ids.front(), QStringLiteral("layers"));
  }

  void EmptySearchListsAllInOrder() {
    snapper::CommandRegistry commands;
    QVERIFY(commands.Add(MakeCommand("b", "Bee")));
    QVERIFY(commands.Add(MakeCommand("a", "Ay")));
    const std::vector<QString> expected = {QStringLiteral("b"),
                                           QStringLiteral("a")};
    QCOMPARE(commands.Search(QStringLiteral("  ")), expected);
  }
};

QTEST_GUILESS_MAIN(CommandRegistryTest)
#include "tst_commandregistry.moc"
