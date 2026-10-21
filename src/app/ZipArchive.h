#pragma once

#include <QByteArray>
#include <QString>

#include <optional>
#include <vector>

struct zip;

namespace snapper {

// Writes a zip file in one go. Entries are kept in memory until Commit()
// writes them all. Destroying it without a commit leaves no file behind.
class ZipWriter {
 public:
  ZipWriter() = default;
  ~ZipWriter();
  ZipWriter(const ZipWriter&) = delete;
  ZipWriter& operator=(const ZipWriter&) = delete;

  // Starts a new file at path, replacing any there on commit.
  bool Open(const QString& path, QString* error);
  bool Add(const QString& name, const QByteArray& data, QString* error);
  bool Commit(QString* error);

 private:
  zip* archive_ = nullptr;
  // libzip reads the bytes only at commit, so they must live until then.
  std::vector<QByteArray> kept_;
};

// Reads entries out of a zip file.
class ZipReader {
 public:
  // Larger entries are refused instead of filling memory.
  static constexpr qint64 kMaxEntryBytes = qint64{1024} * 1024 * 1024;

  ZipReader() = default;
  ~ZipReader();
  ZipReader(const ZipReader&) = delete;
  ZipReader& operator=(const ZipReader&) = delete;

  bool Open(const QString& path, QString* error);
  bool Has(const QString& name) const;
  std::optional<QByteArray> Read(const QString& name, QString* error) const;

 private:
  zip* archive_ = nullptr;
};

}  // namespace snapper
