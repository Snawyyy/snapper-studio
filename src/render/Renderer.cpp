#include "render/Renderer.h"

#include <QPainter>

#include <vector>

#include "document/Document.h"
#include "render/ShapeGeometry.h"

namespace snapper {

void Renderer::DrawPage(QPainter& painter, const Document& document) {
  const CanvasSettings& canvas = document.Canvas();
  painter.fillRect(QRectF(QPointF(0, 0), canvas.size.toSizeF()),
                   canvas.background);
}

void Renderer::DrawLayers(QPainter& painter, const Document& document) {
  const std::vector<Layer>& layers = document.Layers().All();
  const std::vector<Shape>& shapes = document.Shapes().All();

  // Sort shapes into their layers once, keeping their order.
  std::vector<std::vector<const Shape*>> by_layer(layers.size());
  for (const Shape& shape : shapes) {
    const int index = document.Layers().IndexOf(shape.layer);
    if (index >= 0) {
      by_layer[static_cast<size_t>(index)].push_back(&shape);
    }
  }

  for (size_t i = 0; i < layers.size(); ++i) {
    const bool is_drawn =
        layers[i].visible && layers[i].type == LayerType::kVector;
    if (!is_drawn) {
      continue;
    }
    for (const Shape* shape : by_layer[i]) {
      DrawShape(painter, *shape);
    }
  }
}

void Renderer::DrawShape(QPainter& painter, const Shape& shape) {
  const ShapeLook& look = shape.look;
  const bool has_fill = shape.closed && look.has_fill;
  if (has_fill) {
    painter.fillPath(ShapeGeometry::CenterLine(shape), look.fill);
  }
  if (!look.has_line) {
    return;
  }
  const bool is_even = ShapeGeometry::HasEvenWidth(shape);
  if (is_even) {
    const float scale = shape.points.empty() ? 1.0F : shape.points[0].width;
    const QPen pen(look.line, look.line_width * scale, Qt::SolidLine,
                   Qt::RoundCap, Qt::RoundJoin);
    painter.strokePath(ShapeGeometry::CenterLine(shape), pen);
  } else {
    painter.fillPath(ShapeGeometry::LineOutline(shape), look.line);
  }
}

}  // namespace snapper
