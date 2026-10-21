#include <QBuffer>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

#include "app/ProjectIo.h"
#include "document/Document.h"
#include "document/ImageStore.h"

using snapper::ImageStore;

namespace {

QByteArray PngBytes(int width, int height) {
  QImage image(width, height, QImage::Format_ARGB32);
  image.fill(Qt::red);
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  return bytes;
}

}  // namespace

class ImageStoreTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void SamePictureIsStoredOnce() {
    ImageStore store;
    QString error;
    const QString first = store.Add(PngBytes(4, 3), &error).value_or(QString());
    const QString again = store.Add(PngBytes(4, 3), &error).value_or(QString());
    QVERIFY(first.endsWith(QStringLiteral(".png")));
    QCOMPARE(first, again);
    QCOMPARE(store.All().size(), 1);
    QCOMPARE(store.Find(first)->size(), QSize(4, 3));
  }

  void NonPicturesAreRefused() {
    ImageStore store;
    QString error;
    QVERIFY(!store.Add("not a picture", &error).has_value());
    QVERIFY(!error.isEmpty());
  }

  void NamesStayInsideTheFolder() {
    QVERIFY(!ImageStore::IsSafeName(QStringLiteral("../../etc/passwd")));
    QVERIFY(!ImageStore::IsSafeName(QStringLiteral("assets/images/x.png")));
    QVERIFY(ImageStore::IsSafeName(QStringLiteral("assets/images/") +
                                   QString(40, u'a') + QStringLiteral(".png")));
  }

  void PicturesTravelWithTheProject() {
    snapper::Document document;
    document.ResetToNew();
    QString error;
    const QString name =
        document.Images().Add(PngBytes(8, 8), &error).value_or(QString());
    const snapper::Layer layer{.id = document.NewId(),
                               .type = snapper::LayerType::kImage,
                               .name = QStringLiteral("ref"),
                               .image = name,
                               .image_pos = QPointF(5, 6),
                               .image_scale = 2.0};
    document.Layers().Insert(layer, 0);
    // A picture no layer uses is left out.
    document.Images().Add(PngBytes(2, 2), &error);

    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("p.snapper"));
    QVERIFY(
        snapper::ProjectIo::WriteArchive(path, document.Contents(), &error));
    const snapper::LoadedProject loaded =
        snapper::ProjectIo::ReadArchive(path, &error)
            .value_or(snapper::LoadedProject{});
    QCOMPARE(loaded.contents.images.size(), 1);
    snapper::Document reopened;
    QCOMPARE(reopened.Replace(loaded.contents), 0);
    const snapper::Layer* back = reopened.Layers().Find(layer.id);
    QVERIFY(back != nullptr);
    QCOMPARE(back->image_pos, QPointF(5, 6));
    QCOMPARE(back->image_scale, 2.0);
    QVERIFY(reopened.Images().Find(name) != nullptr);
  }
};

QTEST_GUILESS_MAIN(ImageStoreTest)
#include "tst_imagestore.moc"
