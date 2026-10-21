#include "app/ZipArchive.h"

#include <zip.h>

namespace snapper {

namespace {

QString ZipError(zip* archive) {
  return QString::fromUtf8(zip_strerror(archive));
}

QString OpenError(int code) {
  zip_error_t error;
  zip_error_init_with_code(&error, code);
  QString text = QString::fromUtf8(zip_error_strerror(&error));
  zip_error_fini(&error);
  return text;
}

}  // namespace

ZipWriter::~ZipWriter() {
  if (archive_ != nullptr) {
    zip_discard(archive_);
  }
}

bool ZipWriter::Open(const QString& path, QString* error) {
  int code = 0;
  archive_ =
      zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &code);
  if (archive_ == nullptr) {
    *error = QStringLiteral("Can't write %1: %2").arg(path, OpenError(code));
    return false;
  }
  return true;
}

bool ZipWriter::Add(const QString& name, const QByteArray& data,
                    QString* error) {
  if (archive_ == nullptr) {
    *error = QStringLiteral("The file isn't open");
    return false;
  }
  kept_.push_back(data);
  const QByteArray& bytes = kept_.back();
  zip_source_t* source = zip_source_buffer(
      archive_, bytes.constData(), static_cast<zip_uint64_t>(bytes.size()), 0);
  if (source == nullptr) {
    *error = ZipError(archive_);
    return false;
  }
  const zip_int64_t index =
      zip_file_add(archive_, name.toUtf8().constData(), source,
                   ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
  if (index < 0) {
    zip_source_free(source);
    *error = ZipError(archive_);
    return false;
  }
  return true;
}

bool ZipWriter::Commit(QString* error) {
  if (archive_ == nullptr) {
    *error = QStringLiteral("The file isn't open");
    return false;
  }
  if (zip_close(archive_) != 0) {
    *error = ZipError(archive_);
    zip_discard(archive_);
    archive_ = nullptr;
    return false;
  }
  archive_ = nullptr;
  kept_.clear();
  return true;
}

ZipReader::~ZipReader() {
  if (archive_ != nullptr) {
    zip_discard(archive_);
  }
}

bool ZipReader::Open(const QString& path, QString* error) {
  int code = 0;
  archive_ = zip_open(path.toUtf8().constData(), ZIP_RDONLY, &code);
  if (archive_ == nullptr) {
    *error = QStringLiteral("Can't open %1: %2").arg(path, OpenError(code));
    return false;
  }
  return true;
}

bool ZipReader::Has(const QString& name) const {
  return archive_ != nullptr &&
         zip_name_locate(archive_, name.toUtf8().constData(), 0) >= 0;
}

std::optional<QByteArray> ZipReader::Read(const QString& name,
                                          QString* error) const {
  if (archive_ == nullptr) {
    *error = QStringLiteral("The file isn't open");
    return std::nullopt;
  }
  const QByteArray utf8 = name.toUtf8();
  zip_stat_t stat;
  zip_stat_init(&stat);
  const bool has_size = zip_stat(archive_, utf8.constData(), 0, &stat) == 0 &&
                        (stat.valid & ZIP_STAT_SIZE) != 0;
  if (!has_size) {
    *error = QStringLiteral("%1 is missing from the file").arg(name);
    return std::nullopt;
  }
  const bool is_too_big = stat.size > static_cast<zip_uint64_t>(kMaxEntryBytes);
  if (is_too_big) {
    *error = QStringLiteral("%1 is too large to read").arg(name);
    return std::nullopt;
  }
  zip_file_t* file = zip_fopen(archive_, utf8.constData(), 0);
  if (file == nullptr) {
    *error = ZipError(archive_);
    return std::nullopt;
  }
  QByteArray data(static_cast<qsizetype>(stat.size), Qt::Uninitialized);
  const zip_int64_t read =
      zip_fread(file, data.data(), static_cast<zip_uint64_t>(data.size()));
  zip_fclose(file);
  if (read != static_cast<zip_int64_t>(data.size())) {
    *error = QStringLiteral("%1 is damaged").arg(name);
    return std::nullopt;
  }
  return data;
}

}  // namespace snapper
