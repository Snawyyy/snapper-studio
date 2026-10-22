#pragma once

#include "document/Document.h"

namespace snapper::test {

// Adds a style named Ink that sets the line color and brush, and returns
// its id. New projects start with no styles, so tests that need one add it.
inline Id AddInkStyle(Document& document) {
  Style style{.id = document.NewId(), .name = QStringLiteral("Ink")};
  style.parts.line = true;
  style.parts.brush = true;
  document.Styles().Insert(style, document.Styles().Count());
  return style.id;
}

}  // namespace snapper::test
