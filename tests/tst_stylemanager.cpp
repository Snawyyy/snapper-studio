#include <QSignalSpy>
#include <QTest>

#include "document/Document.h"
#include "document/StyleManager.h"

using snapper::ShapeLook;
using snapper::Style;
using snapper::StyleManager;

namespace {

Style Named(snapper::Id id, const char* name) {
  return Style{.id = id, .name = QString::fromLatin1(name)};
}

}  // namespace

class StyleManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void InsertTakeAndSet() {
    StyleManager styles;
    QVERIFY(styles.Insert(Named(1, "a"), 0));
    QVERIFY(styles.Insert(Named(2, "b"), 0));
    QVERIFY(!styles.Insert(Named(2, "again"), 0));
    QVERIFY(!styles.Insert(Named(snapper::kNoId, "none"), 0));
    QCOMPARE(styles.IndexOf(2), 0);

    const QSignalSpy changed(&styles, &StyleManager::StyleChanged);
    Style renamed = *styles.Find(1);
    renamed.name = QStringLiteral("skin");
    QVERIFY(styles.Set(renamed));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(styles.Find(1)->name, QStringLiteral("skin"));
    QVERIFY(!styles.Set(Named(9, "missing")));

    const snapper::RemovedStyle removed =
        styles.Take(2).value_or(snapper::RemovedStyle{.index = -1});
    QCOMPARE(removed.index, 0);
    QCOMPARE(styles.Count(), 1);
  }

  void StopsAtTheLimit() {
    StyleManager styles;
    for (int i = 1; i <= StyleManager::kMaxStyles; ++i) {
      QVERIFY(styles.Insert(Named(i, "s"), i));
    }
    QVERIFY(!styles.Insert(Named(StyleManager::kMaxStyles + 1, "s"), 0));
  }

  // Unset parts come from the shape, and the second style wins over the
  // first.
  void ResolveLayersPartsInOrder() {
    StyleManager styles;
    Style first = Named(1, "first");
    first.parts.line = true;
    first.parts.width = true;
    first.look.line = Qt::red;
    first.look.line_width = 8.0F;
    Style second = Named(2, "second");
    second.parts.line = true;
    second.look.has_line = false;
    second.look.line = Qt::blue;
    styles.Insert(first, 0);
    styles.Insert(second, 1);

    ShapeLook own;
    own.has_fill = true;
    own.fill = Qt::green;
    own.line_width = 2.0F;

    const ShapeLook one = styles.Resolve(own, {1, snapper::kNoId});
    QCOMPARE(one.line, QColor(Qt::red));
    QCOMPARE(one.line_width, 8.0F);
    QCOMPARE(one.fill, QColor(Qt::green));

    const ShapeLook both = styles.Resolve(own, {1, 2});
    QVERIFY(!both.has_line);
    QCOMPARE(both.line, QColor(Qt::blue));
    QCOMPARE(both.line_width, 8.0F);

    const ShapeLook missing = styles.Resolve(own, {7, snapper::kNoId});
    QCOMPARE(missing, own);
  }

  void NewProjectHasADefaultStyle() {
    snapper::Document document;
    document.ResetToNew();
    QCOMPARE(document.Styles().Count(), 1);
    QVERIFY(document.Styles().All().front().parts.line);
  }
};

QTEST_GUILESS_MAIN(StyleManagerTest)
#include "tst_stylemanager.moc"
