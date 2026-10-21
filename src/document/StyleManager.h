#pragma once

#include <QObject>
#include <QString>

#include <cstdint>
#include <optional>
#include <vector>

#include "document/Ids.h"
#include "document/ShapeLook.h"

namespace snapper {

enum class LookPart : std::uint8_t { kFill, kLine, kWidth, kBrush };

// Which parts of its look a style sets. A part it leaves unset comes from
// the style before it, or from the shape itself.
struct StyleParts {
  bool fill = false;   // fill on or off, and its color
  bool line = false;   // line on or off, and its color
  bool width = false;  // line width
  bool brush = false;

  bool operator==(const StyleParts&) const = default;

  bool Sets(LookPart part) const {
    switch (part) {
      case LookPart::kFill:
        return fill;
      case LookPart::kLine:
        return line;
      case LookPart::kWidth:
        return width;
      case LookPart::kBrush:
        return brush;
    }
    return false;
  }
  void Set(LookPart part, bool sets) {
    switch (part) {
      case LookPart::kFill:
        fill = sets;
        break;
      case LookPart::kLine:
        line = sets;
        break;
      case LookPart::kWidth:
        width = sets;
        break;
      case LookPart::kBrush:
        brush = sets;
        break;
    }
  }
};

struct Style {
  Id id = kNoId;
  QString name{};
  ShapeLook look{};
  StyleParts parts{};

  bool operator==(const Style&) const = default;
};

struct RemovedStyle {
  Style style{};
  int index = 0;
};

// Owns the shared styles. Shapes name up to two of them, and editing a
// style changes every shape that uses it.
class StyleManager : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMaxStyles = 1024;

  explicit StyleManager(QObject* parent = nullptr);

  // Inserts at index (clamped). Fails on a missing or repeated id, or a
  // full list.
  bool Insert(const Style& style, int index);
  std::optional<RemovedStyle> Take(Id id);
  // Replaces a style's name, look and parts. False when no style has its
  // id.
  bool Set(const Style& style);
  void Clear();

  const Style* Find(Id id) const;
  // -1 when no style has this id.
  int IndexOf(Id id) const;
  int Count() const { return static_cast<int>(styles_.size()); }
  const std::vector<Style>& All() const { return styles_; }

  // The look a shape draws with: its own, then each part its first style
  // sets, then each part its second style sets. Missing styles are skipped.
  ShapeLook Resolve(const ShapeLook& own, const StylePair& styles) const;

 Q_SIGNALS:
  // A style was added or removed.
  void ListChanged();
  // A style's name, look or parts changed.
  void StyleChanged(Id id);

 private:
  std::vector<Style> styles_;
};

}  // namespace snapper
