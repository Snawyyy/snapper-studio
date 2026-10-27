#include "render/Renderer.h"

#include <QPainter>

#include <vector>

#include "document/Document.h"
#include "eval/Pose.h"
#include "render/BrushStamper.h"
#include "render/ShapeGeometry.h"

namespace snapper {

void Renderer::DrawPage(QPainter& painter, const Document& document) {
  const CanvasSettings& canvas = document.Canvas();
  painter.fillRect(QRectF(QPointF(0, 0), canvas.size.toSizeF()),
                   canvas.background);
}

void Renderer::DrawLayers(QPainter& painter, const Document& document,
                          const Pose& pose) {
  const std::vector<Layer>& layers = document.Layers().All();
  const std::vector<Shape>& shapes = pose.shapes;

  // Sort shapes into their layers once, keeping their order.
  std::vector<std::vector<const Shape*>> by_layer(layers.size());
  for (const Shape& shape : shapes) {
    const int index = document.Layers().IndexOf(shape.layer);
    if (index >= 0) {
      by_layer[static_cast<size_t>(index)].push_back(&shape);
    }
  }

  for (size_t i = 0; i < layers.size(); ++i) {
    const Layer& layer = layers[i];
    if (!document.Layers().IsShown(layer.id)) {
      continue;
    }
    painter.save();
    painter.setTransform(pose.LayerMatrix(layer.id), true);
    if (layer.type == LayerType::kImage) {
      DrawPicture(painter, document, layer);
    }
    for (const Shape* shape : by_layer[i]) {
      DrawShape(painter, document, *shape, pose.frame);
    }
    painter.restore();
  }
}

void Renderer::DrawPicture(QPainter& painter, const Document& document,
                           const Layer& layer) {
  const QImage* image = document.Images().Find(layer.image);
  if (image == nullptr) {
    return;
  }
  const QRectF target(layer.image_pos,
                      image->size().toSizeF() * layer.image_scale);
  painter.save();
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  painter.drawImage(target, *image);
  painter.restore();
}

void Renderer::DrawShape(QPainter& painter, const Document& document,
                         const Shape& shape, int frame) {
  DrawShapeAs(painter, document, shape,
              document.Styles().Resolve(shape.look, shape.styles), frame);
}

void Renderer::DrawShapeAs(QPainter& painter, const Document& document,
                           const Shape& shape, const ShapeLook& look,
                           int frame) {
  const bool has_fill = shape.closed && look.has_fill;
  if (has_fill) {
    painter.fillPath(ShapeGeometry::CenterLine(shape), look.fill);
  }
  if (!look.has_line) {
    return;
  }
  // A brush whose picture is missing draws a plain line.
  const Brush* brush = document.Brushes().Find(look.brush);
  const QImage* picture =
      brush == nullptr ? nullptr : document.Images().Find(brush->image);
  if (picture != nullptr) {
    BrushStamper::Draw(painter, shape, look.line_width, look.line, *brush,
                       *picture, frame);
    return;
  }
  const bool is_even = ShapeGeometry::HasEvenWidth(shape);
  if (is_even) {
    const float scale = shape.points.empty() ? 1.0F : shape.points[0].width;
    const QPen pen(look.line, look.line_width * scale, Qt::SolidLine,
                   Qt::RoundCap, Qt::RoundJoin);
    painter.strokePath(ShapeGeometry::CenterLine(shape), pen);
  } else {
    painter.fillPath(ShapeGeometry::LineOutline(shape, look.line_width),
                     look.line);
  }
}

}  // namespace snapper
