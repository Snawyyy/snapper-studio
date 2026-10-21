#include "edit/tools/PenTool.h"

#include <QLineF>
#include <QPainter>

#include <algorithm>

#include "document/Document.h"
#include "edit/SelectionManager.h"
#include "edit/ShapeCommands.h"
#include "edit/UndoManager.h"
#include "render/Renderer.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

// Distances in screen pixels, so the feel stays the same at any zoom.
constexpr double kMinStepPx = 0.75;
constexpr double kSimplifyPx = 0.8;
constexpr double kCloseGapPx = 12.0;
// The thinnest a line gets under the lightest pressure, as a share of its
// full width.
constexpr float kLightestWidth = 0.15F;

}  // namespace

PenTool::PenTool(ToolContext context) : Tool(std::move(context)) {}

PenTool::~PenTool() = default;

ToolInfo PenTool::Info() const {
  return {.id = QStringLiteral("pen"),
          .title = QStringLiteral("Pen"),
          .key = QKeySequence(Qt::Key_P),
          .hint = QStringLiteral("Pen: draw a line. End it where it began "
                                 "to close the shape.")};
}

void PenTool::SetSettings(const PenSettings& settings) {
  settings_ = settings;
  settings_.smoothing = std::clamp(settings_.smoothing, 0.0, 1.0);
  settings_.line_width =
      std::clamp(settings_.line_width, kMinLineWidth, kMaxLineWidth);
}

void PenTool::Press(const ToolInput& input) {
  const Document& document = Context().document;
  const Layer* layer =
      document.Layers().Find(Context().selection.ActiveLayer());
  const bool is_vector_layer =
      layer != nullptr && layer->type == LayerType::kVector;
  if (!is_vector_layer) {
    Context().hint(QStringLiteral("Pick a vector layer to draw on"));
    return;
  }
  if (!layer->visible) {
    Context().hint(
        QStringLiteral("%1 is hidden: show it to draw on it").arg(layer->name));
    return;
  }
  if (layer->locked) {
    Context().hint(QStringLiteral("%1 is locked: unlock it to draw on it")
                       .arg(layer->name));
    return;
  }
  stroke_ = std::make_unique<PenStroke>(settings_.smoothing,
                                        kMinStepPx * input.canvas_per_screen);
  stroke_->Add(input.canvas, input.pressure);
  pen_pos_ = input.canvas;
  pen_pressure_ = input.pressure;
}

void PenTool::Drag(const ToolInput& input) {
  pen_pos_ = input.canvas;
  pen_pressure_ = input.pressure;
  if (stroke_ != nullptr) {
    stroke_->Add(input.canvas, input.pressure);
  }
}

void PenTool::Release(const ToolInput& input) {
  if (stroke_ == nullptr) {
    return;
  }
  stroke_->Finish(input.canvas, input.pressure);
  const double scale = input.canvas_per_screen;
  std::vector<StrokePoint> points = stroke_->Simplify(kSimplifyPx * scale);
  stroke_.reset();

  const bool is_closed =
      settings_.close_ends && points.size() >= 4 &&
      QLineF(points.front().pos, points.back().pos).length() <
          kCloseGapPx * scale;
  if (is_closed) {
    points.pop_back();
  }
  if (points.size() < 2) {
    return;
  }

  Document& document = Context().document;
  Shape shape;
  shape.id = document.NewId();
  shape.layer = Context().selection.ActiveLayer();
  shape.closed = is_closed;
  shape.look.line_width = static_cast<float>(settings_.line_width);
  shape.look.has_fill = is_closed && settings_.fill_closed;
  for (const StrokePoint& point : points) {
    shape.points.push_back({
        .id = document.NewId(),
        .pos = point.pos,
        .curve = point.is_corner ? 0.0F : kSmoothCurve,
        .width = WidthFor(point.pressure),
    });
  }
  const QString text =
      is_closed ? QStringLiteral("Draw Shape") : QStringLiteral("Draw Line");
  Context().undo.Push(MakeAddShape(document, shape, text));
}

void PenTool::Hover(const ToolInput& input) {
  pen_pos_ = input.canvas;
  is_hovering_ = true;
}

void PenTool::Cancel() {
  stroke_.reset();
  is_hovering_ = false;
}

void PenTool::DrawOverlay(QPainter& painter, const ViewTransform& view) const {
  painter.save();
  if (stroke_ != nullptr) {
    painter.setTransform(view.ToScreenMatrix());
    Renderer::DrawShape(painter, PreviewShape());
  } else if (is_hovering_) {
    // The size of the line about to be drawn, outlined twice so it shows
    // on light and dark art alike.
    const QPointF center = view.ToScreen(pen_pos_);
    const double radius =
        std::max(1.5, settings_.line_width * view.Zoom() / 2.0);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(0, 0, 0, 120), 2.0));
    painter.drawEllipse(center, radius, radius);
    painter.setPen(QPen(QColor(255, 255, 255, 200), 1.0));
    painter.drawEllipse(center, radius, radius);
  }
  painter.restore();
}

Shape PenTool::PreviewShape() const {
  Shape shape;
  shape.look.line_width = static_cast<float>(settings_.line_width);
  Id id = 1;
  for (const StrokeSample& sample : stroke_->Samples()) {
    shape.points.push_back(
        {.id = id++, .pos = sample.pos, .width = WidthFor(sample.pressure)});
  }
  // Reaches all the way to the pen, so the smoothing never shows as lag.
  shape.points.push_back(
      {.id = id, .pos = pen_pos_, .width = WidthFor(pen_pressure_)});
  return shape;
}

float PenTool::WidthFor(float pressure) const {
  if (!settings_.pressure_width) {
    return 1.0F;
  }
  const float clamped = std::clamp(pressure, 0.0F, 1.0F);
  return kLightestWidth + (1.0F - kLightestWidth) * clamped;
}

}  // namespace snapper
