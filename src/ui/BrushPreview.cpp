#include "ui/BrushPreview.h"

#include <QPainter>

#include <cmath>
#include <numbers>

#include "document/Document.h"
#include "render/Renderer.h"

namespace snapper {

namespace {

constexpr int kSamplePoints = 9;

}  // namespace

QImage BrushPreview(const Document& document, Id brush, const QSize& size,
                    const QColor& color, qreal pixel_ratio) {
  QImage image((size.toSizeF() * pixel_ratio).toSize(),
               QImage::Format_ARGB32_Premultiplied);
  image.setDevicePixelRatio(pixel_ratio);
  image.fill(Qt::transparent);

  const double width = size.width();
  const double height = size.height();
  const double line_width = height * 0.45;
  const double margin = line_width;
  Shape stroke;
  stroke.id = 1;
  for (int i = 0; i < kSamplePoints; ++i) {
    const double t = static_cast<double>(i) / (kSamplePoints - 1);
    const double x = margin + t * (width - 2 * margin);
    const double y =
        height / 2 + std::sin(t * 2 * std::numbers::pi) * height * 0.2;
    stroke.points.push_back(
        {.id = i + 2,
         .pos = QPointF(x, y),
         .width =
             static_cast<float>(0.25 + 0.75 * std::sin(t * std::numbers::pi))});
  }
  ShapeLook look;
  look.line = color;
  look.line_width = static_cast<float>(line_width);
  look.brush = brush;
  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing);
  Renderer::DrawShapeAs(painter, document, stroke, look);
  return image;
}

}  // namespace snapper
