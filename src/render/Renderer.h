#pragma once

class QPainter;

namespace snapper {

class Document;
struct Shape;

// Draws the artwork: the canvas page and every visible layer, bottom to
// top. The painter is already set to canvas coordinates.
class Renderer {
 public:
  static void DrawPage(QPainter& painter, const Document& document);
  static void DrawLayers(QPainter& painter, const Document& document);
  static void DrawShape(QPainter& painter, const Shape& shape);
};

}  // namespace snapper
