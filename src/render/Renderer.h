#pragma once

class QPainter;

namespace snapper {

class Document;
struct Layer;
struct Pose;
struct Shape;
struct ShapeLook;

// Draws the artwork: the canvas page and every visible layer, bottom to
// top, as posed on one frame. The painter is already set to canvas
// coordinates. `frame` picks the stamps of boiling brushes.
class Renderer {
 public:
  static void DrawPage(QPainter& painter, const Document& document);
  static void DrawLayers(QPainter& painter, const Document& document,
                         const Pose& pose);
  // In the look its own look and styles give it.
  static void DrawShape(QPainter& painter, const Document& document,
                        const Shape& shape, int frame = 0);
  // In the given look, with the document's brushes.
  static void DrawShapeAs(QPainter& painter, const Document& document,
                          const Shape& shape, const ShapeLook& look,
                          int frame = 0);
  static void DrawPicture(QPainter& painter, const Document& document,
                          const Layer& layer);
};

}  // namespace snapper
