#include "document/ImageStore.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QImageReader>
#include <QRegularExpression>

namespace snapper {

namespace {

const QString kFolder = QStringLiteral("assets/images/");

}  // namespace

std::optional<ImageStore::Image> ImageStore::Decode(const QByteArray& bytes,
                                                    QString* error) {
  if (bytes.size() > kMaxBytes) {
    *error = QStringLiteral("The picture is larger than 256 MB");
    return std::nullopt;
  }
  QBuffer buffer;
  buffer.setData(bytes);
  QImageReader reader(&buffer);
  const QSize size = reader.size();
  const bool is_too_big = size.width() > kMaxSide || size.height() > kMaxSide;
  if (is_too_big) {
    *error =
        QStringLiteral("The picture is over %1 pixels on a side").arg(kMaxSide);
    return std::nullopt;
  }
  Image image{.bytes = bytes, .pixels = reader.read()};
  if (image.pixels.isNull()) {
    *error =
        QStringLiteral("That file isn't a picture Snapper Studio can read");
    return std::nullopt;
  }
  return image;
}

std::optional<QString> ImageStore::Add(const QByteArray& bytes,
                                       QString* error) {
  QBuffer buffer;
  buffer.setData(bytes);
  buffer.open(QIODevice::ReadOnly);
  const QByteArray format = QImageReader::imageFormat(&buffer);
  const QString hash = QString::fromLatin1(
      QCryptographicHash::hash(bytes, QCryptographicHash::Sha1).toHex());
  QString name =
      kFolder + hash + u'.' +
      (format.isEmpty() ? QStringLiteral("img") : QString::fromLatin1(format));
  if (images_.contains(name)) {
    return name;
  }
  if (!AddNamed(name, bytes, error)) {
    return std::nullopt;
  }
  return name;
}

bool ImageStore::AddNamed(const QString& name, const QByteArray& bytes,
                          QString* error) {
  if (!IsSafeName(name)) {
    *error = QStringLiteral("%1 isn't a picture name").arg(name);
    return false;
  }
  if (images_.contains(name)) {
    return true;
  }
  if (images_.size() >= kMaxImages) {
    *error =
        QStringLiteral("A project holds at most %1 pictures").arg(kMaxImages);
    return false;
  }
  std::optional<Image> image = Decode(bytes, error);
  if (!image.has_value()) {
    return false;
  }
  images_.insert(name, std::move(*image));
  return true;
}

const QImage* ImageStore::Find(const QString& name) const {
  const auto found = images_.constFind(name);
  return found == images_.cend() ? nullptr : &found->pixels;
}

// Only plain names inside the image folder, so a file can never point
// outside it.
bool ImageStore::IsSafeName(const QString& name) {
  static const QRegularExpression kPattern(
      QStringLiteral("^assets/images/[0-9a-f]{40}\\.[a-z0-9]{1,8}$"));
  return kPattern.match(name).hasMatch();
}

}  // namespace snapper
