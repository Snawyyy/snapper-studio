#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <optional>

#include "document/Document.h"

namespace snapper {

struct LoadedProject {
  DocumentContents contents;
  // What was dropped while loading, for the user to read.
  QStringList warnings;
};

// Turns a document into the readable project.json and back. See
// FILE_FORMAT.md. Reading checks every id link: a broken one is dropped
// with a warning, and the project still opens.
class ProjectJson {
 public:
  static constexpr int kVersion = 1;
  static constexpr int kMaxWarnings = 20;

  static QByteArray Write(const DocumentContents& contents);

  // Fails, with the reason, when the text isn't a project, comes from a
  // newer version, or is over the limits.
  static std::optional<LoadedProject> Read(const QByteArray& json,
                                           QString* error);
};

}  // namespace snapper
