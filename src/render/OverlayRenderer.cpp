#include "render/OverlayRenderer.h"

#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>

#include <algorithm>
#include <cmath>

#include "app/Theme.h"
#include "document/Document.h"
#include "eval/Pose.h"
#include "render/ShapeGeometry.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

// The area around the page covers artwork drawn off the page, faded.
constexpr int kOffPageCoverAlpha = 170;
constexpr int kShadowLayers = 4;
constexpr int kShadowAlpha = 18;
constexpr double kHighlightWidth = 1.5;
// Screen pixels between a line's edge and its highlight ring.
constexpr double kHighlightGap = 3.0;
// A bone's kite is widest this far along, at this share of its length,
// within these screen sizes.
constexpr double kBoneWaist = 0.15;
constexpr double kBoneWidthShare = 0.12;
constexpr double kBoneMinWidthPx = 4.0;
constexpr double kBoneMaxWidthPx = 14.0;
constexpr double kPinRadiusPx = 6.0;
constexpr double kFadedOpacity = 0.4;
constexpr double kModeBorderPx = 3.0;
constexpr int kLabelPaddingPx = 8;
constexpr int kLabelInsetPx = 12;

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

void OverlayRenderer::DrawBones(QPainter& painter, const Pose& pose,
                                const ViewTransform& view, Id bone_layer,
                                const QSet<Id>& selected, bool is_faded,
                                const QSet<Id>& pinned, const Theme& theme) {
  const QTransform to_screen =
      pose.LayerMatrix(bone_layer) * view.ToScreenMatrix();
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  if (is_faded) {
    painter.setOpacity(kFadedOpacity);
  }
  const QColor outline(0, 0, 0, 150);
  const QColor fill(255, 255, 255, 190);
  for (auto it = pose.bones.cbegin(); it != pose.bones.cend(); ++it) {
    if (it->layer != bone_layer) {
      continue;
    }
    const QPointF base = to_screen.map(it->Base());
    const QPointF tip = to_screen.map(it->Tip());
    const QPointF span = tip - base;
    const double length = std::hypot(span.x(), span.y());
    if (length < 1e-6) {
      continue;
    }
    const QPointF side = QPointF(-span.y(), span.x()) / length;
    const double half_width =
        std::clamp(length * kBoneWidthShare, kBoneMinWidthPx, kBoneMaxWidthPx) /
        2;
    const QPointF waist = base + span * kBoneWaist;
    const QPolygonF kite = {base, waist + side * half_width, tip,
                            waist - side * half_width};
    const bool is_selected = selected.contains(it.key());
    painter.setPen(QPen(outline, 1.0));
    painter.setBrush(is_selected ? theme.accent : fill);
    painter.drawPolygon(kite);
    if (pinned.contains(it.key())) {
      painter.setBrush(Qt::NoBrush);
      painter.setPen(QPen(theme.setup_mode, 2.0));
      painter.drawEllipse(base, kPinRadiusPx, kPinRadiusPx);
    }
  }
  painter.restore();
}

void OverlayRenderer::DrawModeBorder(QPainter& painter, const QRect& area,
                                     const QColor& color, const QString& label,
                                     const Theme& theme) {
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  const double half = kModeBorderPx / 2;
  painter.setPen(QPen(color, kModeBorderPx));
  painter.setBrush(Qt::NoBrush);
  painter.drawRect(QRectF(area).adjusted(half, half, -half, -half));

  QFont font = painter.font();
  font.setPixelSize(theme.font_small);
  font.setWeight(QFont::DemiBold);
  painter.setFont(font);
  const QFontMetrics metrics(font);
  const QRect text = metrics.boundingRect(label);
  const QRectF pill(area.left() + kLabelInsetPx, area.top() + kLabelInsetPx,
                    text.width() + 2 * kLabelPaddingPx,
                    metrics.height() + kLabelPaddingPx);
  painter.setPen(Qt::NoPen);
  painter.setBrush(color);
  painter.drawRoundedRect(pill, theme.radius_pill, theme.radius_pill);
  painter.setPen(QColor(0, 0, 0, 220));
  painter.drawText(pill, Qt::AlignCenter, label);
  painter.restore();
}

// A ring round each line, never over it, so the change itself stays in
// plain sight.
void OverlayRenderer::DrawStyleUsers(QPainter& painter,
                                     const Document& document, const Pose& pose,
                                     const ViewTransform& view, Id style,
                                     const QColor& color) {
  if (style == kNoId) {
    return;
  }
  painter.save();
  painter.setBrush(Qt::NoBrush);
  painter.setPen(QPen(color, kHighlightWidth));
  for (const Id id : document.Shapes().ShapesWithStyle(style)) {
    const Shape* shape = pose.FindShape(id);
    const Layer* layer =
        shape == nullptr ? nullptr : document.Layers().Find(shape->layer);
    const bool is_shown =
        layer != nullptr && document.Layers().IsShown(layer->id);
    if (!is_shown) {
      continue;
    }
    const ShapeLook look =
        document.Styles().Resolve(shape->look, shape->styles);
    QPainterPathStroker stroker;
    const QTransform to_screen =
        pose.LayerMatrix(layer->id) * view.ToScreenMatrix();
    const double scale = std::sqrt(std::abs(to_screen.determinant()));
    stroker.setWidth(look.line_width * scale + 2 * kHighlightGap);
    stroker.setJoinStyle(Qt::RoundJoin);
    stroker.setCapStyle(Qt::RoundCap);
    const QPainterPath line = to_screen.map(ShapeGeometry::CenterLine(*shape));
    painter.drawPath(stroker.createStroke(line).simplified());
  }
  painter.restore();
}

}  // namespace snapper
