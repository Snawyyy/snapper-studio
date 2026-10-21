#include "edit/tools/ShapeTool.h"

#include <QPainter>

#include <algorithm>
#include <array>
#include <cmath>

#include "document/Document.h"
#include "edit/SelectionManager.h"
#include "edit/ShapeCommands.h"
#include "edit/UndoManager.h"
#include "edit/tools/LayerCheck.h"
#include "render/Renderer.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

constexpr double kMinSize = 1.0;

}  // namespace

ShapeTool::ShapeTool(ToolContext context, Kind kind)
    : Tool(std::move(context)), kind_(kind) {}

ToolInfo ShapeTool::Info() const {
  if (kind_ == Kind::kRectangle) {
    return {.id = QStringLiteral("rectangle"),
            .title = QStringLiteral("Rectangle"),
            .key = QKeySequence(Qt::Key_M),
            .hint = QStringLiteral("Rectangle: drag from corner to corner. "
                                   "Shift for a square, Alt from the middle.")};
  }
  return {.id = QStringLiteral("ellipse"),
          .title = QStringLiteral("Ellipse"),
          .key = QKeySequence(Qt::Key_E),
          .hint = QStringLiteral("Ellipse: drag from corner to corner. "
                                 "Shift for a circle, Alt from the middle.")};
}

void ShapeTool::SetSettings(const ShapeToolSettings& settings) {
  settings_ = settings;
  settings_.line_width = std::clamp(settings_.line_width, 0.5, 200.0);
}

void ShapeTool::Press(const ToolInput& input) {
  if (!CanChangeActiveLayer(Context(), QStringLiteral("draw on"))) {
    return;
  }
  is_dragging_ = true;
  start_ = input.canvas;
  end_ = input.canvas;
  modifiers_ = input.modifiers;
}

void ShapeTool::Drag(const ToolInput& input) {
  end_ = input.canvas;
  modifiers_ = input.modifiers;
}

void ShapeTool::Release(const ToolInput& input) {
  if (!is_dragging_) {
    return;
  }
  is_dragging_ = false;
  end_ = input.canvas;
  modifiers_ = input.modifiers;
  const QRectF rect = DraggedRect();
  const bool is_too_small = rect.width() < kMinSize || rect.height() < kMinSize;
  if (is_too_small) {
    return;
  }
  const QString text = kind_ == Kind::kRectangle
                           ? QStringLiteral("Draw Rectangle")
                           : QStringLiteral("Draw Ellipse");
  Context().undo.Push(
      MakeAddShape(Context().document, Build(rect, true), text));
}

void ShapeTool::DrawOverlay(QPainter& painter,
                            const ViewTransform& view) const {
  if (!is_dragging_) {
    return;
  }
  painter.save();
  painter.setTransform(view.ToScreenMatrix());
  // Building without ids changes nothing in the document.
  Renderer::DrawShape(painter, Build(DraggedRect(), false));
  painter.restore();
}

QRectF ShapeTool::DraggedRect() const {
  QPointF delta = end_ - start_;
  const bool is_even = modifiers_.testFlag(Qt::ShiftModifier);
  if (is_even) {
    const double side = std::max(std::abs(delta.x()), std::abs(delta.y()));
    delta =
        QPointF(std::copysign(side, delta.x()), std::copysign(side, delta.y()));
  }
  const bool is_from_middle = modifiers_.testFlag(Qt::AltModifier);
  if (is_from_middle) {
    return QRectF(start_ - delta, start_ + delta).normalized();
  }
  return QRectF(start_, start_ + delta).normalized();
}

Shape ShapeTool::Build(const QRectF& rect, bool with_ids) const {
  Document& document = Context().document;
  const auto new_id = [&] { return with_ids ? document.NewId() : kNoId; };
  Shape shape;
  shape.id = new_id();
  shape.layer = Context().selection.ActiveLayer();
  shape.closed = true;
  shape.look.has_fill = settings_.has_fill;
  shape.look.fill = QColor::fromRgba(kDefaultFill);
  shape.look.has_line = settings_.has_line;
  shape.look.line_width = static_cast<float>(settings_.line_width);

  const bool is_rectangle = kind_ == Kind::kRectangle;
  const QPointF c = rect.center();
  // Corners clockwise from the top left, or the ends of the axes clockwise
  // from the top.
  const std::array<QPointF, 4> corners =
      is_rectangle
          ? std::array<QPointF, 4>{rect.topLeft(), rect.topRight(),
                                   rect.bottomRight(), rect.bottomLeft()}
          : std::array<QPointF, 4>{
                QPointF(c.x(), rect.top()), QPointF(rect.right(), c.y()),
                QPointF(c.x(), rect.bottom()), QPointF(rect.left(), c.y())};
  const float curve = is_rectangle ? 0.0F : kEllipseCurve;
  for (const QPointF& corner : corners) {
    shape.points.push_back({.id = new_id(), .pos = corner, .curve = curve});
  }
  return shape;
}

}  // namespace snapper
