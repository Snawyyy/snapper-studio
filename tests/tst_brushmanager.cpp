#include <QImage>
#include <QSignalSpy>
#include <QTest>

#include "document/BrushManager.h"
#include "document/Document.h"
#include "document/StandardBrushes.h"

using snapper::Brush;
using snapper::BrushManager;

class BrushManagerTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void InsertClampsSettings() {
    BrushManager brushes;
    QVERIFY(brushes.Insert({.id = 1, .spacing = 0.0F, .boil_every = -3}, 0));
    QCOMPARE(brushes.Find(1)->spacing, Brush::kMinSpacing);
    QCOMPARE(brushes.Find(1)->boil_every, 0);
    QVERIFY(!brushes.Insert({.id = 1}, 0));
    QVERIFY(!brushes.Insert({.id = snapper::kNoId}, 0));
  }

  void SetChangesSettingsAndSignals() {
    BrushManager brushes;
    brushes.Insert({.id = 4, .name = QStringLiteral("a")}, 0);
    const QSignalSpy changed(&brushes, &BrushManager::BrushChanged);
    QVERIFY(brushes.Set({.id = 4, .spacing = 9.0F, .size_jitter = 0.5F}));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(brushes.Find(4)->spacing, Brush::kMaxSpacing);
    QCOMPARE(brushes.Find(4)->size_jitter, 0.5F);
    QVERIFY(!brushes.Set({.id = 5}));
    QVERIFY(brushes.Take(4).has_value());
    QCOMPARE(brushes.Count(), 0);
  }

  // The same pictures every time, so projects store each only once.
  void StandardTipsRepeat() {
    const auto first = snapper::StandardBrushes();
    const auto second = snapper::StandardBrushes();
    QCOMPARE(first.size(), second.size());
    for (size_t i = 0; i < first.size(); ++i) {
      QCOMPARE(first[i].png, second[i].png);
      const QImage tip = QImage::fromData(first[i].png);
      QVERIFY(!tip.isNull());
      QVERIFY(tip.hasAlphaChannel());
    }
  }

  void NewProjectHasStandardBrushesWithPictures() {
    snapper::Document document;
    document.ResetToNew();
    QCOMPARE(document.Brushes().Count(),
             static_cast<int>(snapper::StandardBrushes().size()));
    for (const Brush& brush : document.Brushes().All()) {
      QVERIFY(document.Images().Find(brush.image) != nullptr);
    }
    // Saved contents carry the brush pictures.
    QCOMPARE(document.Contents().images.size(), document.Brushes().Count());
  }
};

QTEST_GUILESS_MAIN(BrushManagerTest)
#include "tst_brushmanager.moc"
