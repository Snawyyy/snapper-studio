#pragma once

#include <QByteArray>
#include <QHash>
#include <QImage>
#include <QString>

#include <optional>

namespace snapper {

// Owns the pictures a project uses. Each is kept as its original file
// bytes, named by content hash, so a picture imported twice is stored once
// and a project never breaks when moved.
class ImageStore {
 public:
  static constexpr int kMaxImages = 256;
  static constexpr int kMaxSide = 16384;
  static constexpr qint64 kMaxBytes = qint64{256} * 1024 * 1024;

  struct Image {
    QByteArray bytes;
    QImage pixels;
  };

  // Adds a picture from its file bytes and returns its name inside the
  // project, "assets/images/<sha1>.<format>". Fails, with the reason, on
  // bytes that aren't a picture, a picture too large, or a full store.
  std::optional<QString> Add(const QByteArray& bytes, QString* error);
  // Adds a picture under the name it was saved with. Fails on a name
  // outside assets/images/ or bytes that aren't a picture.
  bool AddNamed(const QString& name, const QByteArray& bytes, QString* error);
  void Clear() { images_.clear(); }

  const QImage* Find(const QString& name) const;
  const QHash<QString, Image>& All() const { return images_; }

  static bool IsSafeName(const QString& name);

 private:
  static std::optional<Image> Decode(const QByteArray& bytes, QString* error);

  QHash<QString, Image> images_;
};

}  // namespace snapper
