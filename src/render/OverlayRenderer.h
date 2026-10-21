#pragma once

#include <QColor>
#include <QRect>

#include "document/Ids.h"

class QPainter;

namespace snapper {

class Document;
class ViewTransform;

// Draws the editing aids over the artwork, in screen coordinates.
class OverlayRenderer {
 public:
  // Fades artwork that lies off the page and draws a soft shadow round
  // the page. `cover` is the color around the canvas.
  static void DrawSurround(QPainter& painter, const Document& document,
                           const ViewTransform& view, const QRect& area,
                           const QColor& cover);
  // Outlines every shape using the style, so you see what editing it
  // changes.
  static void DrawStyleUsers(QPainter& painter, const Document& document,
                             const ViewTransform& view, Id style,
                             const QColor& color);
};

}  // namespace snapper
