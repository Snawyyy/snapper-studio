#pragma once

#include <QJsonObject>
#include <QSet>

#include <optional>

#include "document/BrushManager.h"
#include "document/Ids.h"
#include "document/JsonWarnings.h"
#include "document/StyleManager.h"

namespace snapper {

// Turns styles and brushes into their project.json objects and back. See
// FILE_FORMAT.md. A style writes only the parts it sets.
class LookJson {
 public:
  static QJsonObject WriteStyle(const Style& style);
  static QJsonObject WriteBrush(const Brush& brush);
  // A color, or null when it's turned off.
  static QJsonValue WriteColor(bool is_on, const QColor& color);

  // Nothing when the object can't be used, with a warning. Ids already in
  // `taken` count as bad. A link to a brush outside `brushes` is dropped,
  // leaving a plain line.
  static std::optional<Brush> ReadBrush(const QJsonObject& json,
                                        const QSet<Id>& taken,
                                        JsonWarnings& warnings);
  static std::optional<Style> ReadStyle(const QJsonObject& json,
                                        const QSet<Id>& taken,
                                        const QSet<Id>& brushes,
                                        JsonWarnings& warnings);
  // Sets `is_on` and `color` from a color or null. A bad value leaves the
  // color as it was.
  static void ReadColor(const QJsonValue& value, bool& is_on, QColor& color);
};

}  // namespace snapper
