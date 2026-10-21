#include "render/OverlayRenderer.h"

#include <QPainter>
#include <QPainterPath>

#include "document/Document.h"
#include "render/ShapeGeometry.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

// The area around the page covers artwork drawn off the page, faded.
constexpr int kOffPageCoverAlpha = 170;
constexpr int kShadowLayers = 4;
constexpr int kShadowAlpha = 18;
constexpr double kHighlightWidth = 2.0;

}  // namespace

void OverlayRenderer::DrawSurround(QPainter& painter, const Document& document,
                                   const ViewTransform& view, const QRect& area,
                                   const QColor& cover) {
  const QSizeF size = document.Canvas().size.toSizeF();
  const QPolygonF page =
      view.ToScreenMatrix().map(QPolygonF(QRectF(QPointF(0, 0), size)));
  QPainterPath outside;
  outside.addRect(area);
  QPainterPath page_path;
  page_path.addPolygon(page);
  page_path.closeSubpath();

  QColor faded = cover;
  faded.setAlpha(kOffPageCoverAlpha);
  const QPainterPath off_page = outside.subtracted(page_path);
  painter.fillPath(off_page, faded);

  // The shadow falls only outside the page.
  painter.save();
  painter.setClipPath(off_page);
  painter.setBrush(Qt::NoBrush);
  for (int i = 1; i <= kShadowLayers; ++i) {
    painter.setPen(QPen(QColor(0, 0, 0, kShadowAlpha), i * 2.0));
    painter.drawPolygon(page.translated(0, i * 0.5));
  }
  painter.restore();
}

void OverlayRenderer::DrawStyleUsers(QPainter& painter,
                                     const Document& document,
                                     const ViewTransform& view, Id style,
                                     const QColor& color) {
  if (style == kNoId) {
    return;
  }
  const QTransform to_screen = view.ToScreenMatrix();
  painter.save();
  painter.setBrush(Qt::NoBrush);
  painter.setPen(QPen(color, kHighlightWidth));
  for (const Id id : document.Shapes().ShapesWithStyle(style)) {
    const Shape* shape = document.Shapes().Find(id);
    const Layer* layer = document.Layers().Find(shape->layer);
    const bool is_shown = layer != nullptr && layer->visible;
    if (is_shown) {
      painter.drawPath(to_screen.map(ShapeGeometry::CenterLine(*shape)));
    }
  }
  painter.restore();
}

}  // namespace snapper
