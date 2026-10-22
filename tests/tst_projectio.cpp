#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include "TestStyles.h"
#include "app/ProjectIo.h"
#include "document/Document.h"
#include "document/ProjectJson.h"

using snapper::DocumentContents;
using snapper::LoadedProject;
using snapper::ProjectIo;
using snapper::ProjectJson;

namespace {

DocumentContents Sample() {
  snapper::Document document;
  document.ResetToNew();
  const snapper::Id layer = document.Layers().All().front().id;
  snapper::Shape shape;
  shape.id = document.NewId();
  shape.layer = layer;
  shape.closed = true;
  shape.look.has_fill = true;
  shape.look.fill = QColor(10, 20, 30);
  shape.look.line_width = 4.5F;
  for (int i = 0; i < 3; ++i) {
    shape.points.push_back({.id = document.NewId(),
                            .pos = QPointF(i * 10.5, i),
                            .curve = 0.0F,
                            .width = 0.5F});
  }
  const snapper::Brush& chalk = document.Brushes().All()[2];
  shape.look.brush = chalk.id;
  shape.styles = {snapper::test::AddInkStyle(document), snapper::kNoId};
  document.Shapes().Insert(shape, 0);
  document.Layers().SetLocked(layer, true);
  snapper::Style skin{.id = document.NewId(), .name = QStringLiteral("Skin")};
  skin.parts.fill = true;
  skin.parts.width = true;
  skin.parts.brush = true;
  skin.look.has_fill = true;
  skin.look.fill = QColor(200, 150, 120);
  skin.look.line_width = 7.0F;
  skin.look.brush = chalk.id;
  document.Styles().Insert(skin, 1);
  return document.Contents();
}

QJsonObject Parse(const QByteArray& json) {
  return QJsonDocument::fromJson(json).object();
}

}  // namespace

class ProjectIoTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void RoundTripKeepsEverything() {
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("a.snapper"));
    const DocumentContents saved = Sample();
    QString error;
    QVERIFY2(ProjectIo::WriteArchive(path, saved, &error), qPrintable(error));
    QVERIFY(!QFile::exists(path + QStringLiteral(".tmp")));

    const LoadedProject loaded =
        ProjectIo::ReadArchive(path, &error).value_or(LoadedProject{});
    QVERIFY2(error.isEmpty(), qPrintable(error));
    const DocumentContents& back = loaded.contents;
    QVERIFY(loaded.warnings.isEmpty());
    QCOMPARE(back.next_id, saved.next_id);
    QCOMPARE(back.layers.size(), size_t{1});
    QVERIFY(back.layers[0].locked);
    QCOMPARE(back.shapes.size(), size_t{1});
    const snapper::Shape& shape = back.shapes[0];
    QVERIFY(shape.closed);
    QCOMPARE(shape.look.fill, QColor(10, 20, 30));
    QCOMPARE(shape.look.line_width, 4.5F);
    QCOMPARE(shape.points.size(), size_t{3});
    QCOMPARE(shape.points[2].pos, QPointF(21, 2));
    QCOMPARE(shape.points[2].width, 0.5F);
    QCOMPARE(shape.points[2].curve, 0.0F);

    QCOMPARE(back.styles, saved.styles);
    QCOMPARE(back.brushes, saved.brushes);
    QCOMPARE(shape.styles, saved.shapes[0].styles);
    QCOMPARE(shape.look.brush, saved.shapes[0].look.brush);
    for (const snapper::Brush& brush : back.brushes) {
      QVERIFY(back.images.contains(brush.image));
    }
  }

  void MissingStyleLinksAreDroppedWithAWarning() {
    QJsonObject root = Parse(ProjectJson::Write(Sample()));
    root[QStringLiteral("styles")] = QJsonArray{};
    QString error;
    const LoadedProject loaded =
        ProjectJson::Read(QJsonDocument(root).toJson(), &error)
            .value_or(LoadedProject{});
    QCOMPARE(loaded.contents.shapes.size(), size_t{1});
    QCOMPARE(loaded.contents.shapes[0].styles,
             (snapper::StylePair{snapper::kNoId, snapper::kNoId}));
    QCOMPARE(loaded.warnings.size(), 1);
  }

  // A project from before brushes gets the standard ones, and no styles.
  void Version1GetsBrushes() {
    QJsonObject root = Parse(ProjectJson::Write(Sample()));
    root[QStringLiteral("version")] = 1;
    root.remove(QStringLiteral("styles"));
    root.remove(QStringLiteral("brushes"));
    QJsonArray shapes = root.value(QStringLiteral("shapes")).toArray();
    QJsonObject shape = shapes[0].toObject();
    shape.remove(QStringLiteral("styles"));
    shape.remove(QStringLiteral("brush"));
    shapes[0] = shape;
    root[QStringLiteral("shapes")] = shapes;
    QString error;
    const LoadedProject loaded =
        ProjectJson::Read(QJsonDocument(root).toJson(), &error)
            .value_or(LoadedProject{});
    QVERIFY(loaded.warnings.isEmpty());
    const DocumentContents& back = loaded.contents;
    QVERIFY(back.styles.empty());
    QVERIFY(!back.brushes.empty());
    QCOMPARE(back.images.size(), static_cast<qsizetype>(back.brushes.size()));
    QVERIFY(back.next_id > back.brushes.back().id);
    snapper::Document document;
    QCOMPARE(document.Replace(back), 0);
  }

  void BrokenLinksAreDroppedWithAWarning() {
    QJsonObject root = Parse(ProjectJson::Write(Sample()));
    QJsonArray shapes = root.value(QStringLiteral("shapes")).toArray();
    QJsonObject orphan = shapes[0].toObject();
    orphan[QStringLiteral("id")] = 9000;
    orphan[QStringLiteral("layer")] = 12345;
    shapes.append(orphan);
    root[QStringLiteral("shapes")] = shapes;

    QString error;
    const LoadedProject loaded =
        ProjectJson::Read(QJsonDocument(root).toJson(), &error)
            .value_or(LoadedProject{});
    QCOMPARE(loaded.contents.shapes.size(), size_t{1});
    QCOMPARE(loaded.warnings.size(), 1);
  }

  void NextIdStaysAboveEveryId() {
    QJsonObject root = Parse(ProjectJson::Write(Sample()));
    root[QStringLiteral("next_id")] = 1;
    QString error;
    const LoadedProject loaded =
        ProjectJson::Read(QJsonDocument(root).toJson(), &error)
            .value_or(LoadedProject{});
    QCOMPARE(loaded.contents.shapes.size(), size_t{1});
    QVERIFY(loaded.contents.next_id > loaded.contents.shapes[0].id);
  }

  void NewerVersionIsRefused() {
    QJsonObject root = Parse(ProjectJson::Write(Sample()));
    root[QStringLiteral("version")] = ProjectJson::kVersion + 1;
    QString error;
    QVERIFY(!ProjectJson::Read(QJsonDocument(root).toJson(), &error));
    QVERIFY(error.contains(QStringLiteral("newer")));
  }

  void NotAProjectIsRefused() {
    QString error;
    QVERIFY(!ProjectJson::Read("{\"hello\": 1}", &error));
    QVERIFY(!error.isEmpty());
    const QTemporaryDir dir;
    QVERIFY(
        !ProjectIo::ReadArchive(dir.filePath(QStringLiteral("none")), &error));
  }
};

QTEST_GUILESS_MAIN(ProjectIoTest)
#include "tst_projectio.moc"
