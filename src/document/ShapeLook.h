#pragma once

#include <QColor>

#include <array>

#include "document/Ids.h"

namespace snapper {

// How a shape draws: its fill, its line, and the brush the line is stamped
// with. kNoId as the brush draws a plain line.
struct ShapeLook {
  bool has_fill = false;
  QColor fill = Qt::white;
  bool has_line = true;
  QColor line = Qt::black;
  float line_width = 3.0F;
  Id brush = kNoId;

  bool operator==(const ShapeLook&) const = default;
};

// A shape uses up to two shared styles, kNoId where it uses none.
constexpr int kStylesPerShape = 2;
using StylePair = std::array<Id, kStylesPerShape>;

}  // namespace snapper
