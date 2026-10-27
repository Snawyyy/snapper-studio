#include "edit/tools/TransformLayerTool.h"

#include <QLineF>
#include <QPainter>
#include <QPolygonF>
#include <QtMath>

#include <algorithm>
#include <cmath>

#include "app/Theme.h"
#include "document/Document.h"
#include "edit/AnimationEditor.h"
#include "edit/SelectionManager.h"
#include "edit/ShapeCommands.h"
#include "edit/UndoManager.h"
#include "eval/FrameEvaluator.h"
#include "render/ShapeGeometry.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

constexpr double kHandlePx = 8.0;
// How far outside a corner still turns the layer.
constexpr double kTurnReachPx = 28.0;
constexpr double kSnapAngle = M_PI / 12;
constexpr double kMinScale = 0.01;
constexpr double kOriginPx = 6.0;

std::array<QPointF, 4> CornersOf(const QRectF& box) {
  return {box.topLeft(), box.topRight(), box.bottomRight(), box.bottomLeft()};
}

double KeepAwayFromZero(double scale) {
  return std::abs(scale) < kMinScale ? std::copysign(kMinScale, scale) : scale;
}

}  // namespace

TransformLayerTool::TransformLayerTool(ToolContext context)
    : Tool(std::move(context)) {}

ToolInfo TransformLayerTool::Info() const {
  return {.id = QStringLiteral("transform-layer"),
          .title = QStringLiteral("Transform Layer"),
          .key = QKeySequence(Qt::Key_T),
          .hint = QStringLiteral(
              "Transform Layer: drag inside to move, a corner to scale, just "
              "outside a corner to turn. Shift keeps it even.")};
}

std::optional<QRectF> TransformLayerTool::ContentBox() const {
  const Document& document = Context().document;
  const Layer* layer =
      document.Layers().Find(Context().selection.ActiveLayer());
  if (layer == nullptr) {
    return std::nullopt;
  }
  if (layer->type == LayerType::kImage) {
    const QImage* image = document.Images().Find(layer->image);
    if (image == nullptr) {
      return std::nullopt;
    }
    return QRectF(layer->image_pos,
                  image->size().toSizeF() * layer->image_scale);
  }
  std::optional<QRectF> box;
  for (const Shape& shape : Context().frames.Current().shapes) {
    if (shape.layer != layer->id) {
      continue;
    }
    const QRectF bounds = ShapeGeometry::CenterLine(shape).boundingRect();
    box = box.has_value() ? box->united(bounds) : bounds;
  }
  return box;
}

bool TransformLayerTool::IsUntouched(const Layer& layer) const {
  const bool has_channels = std::ranges::any_of(
      AnimationEditor::kLayerProperties, [&](ChannelProperty property) {
        return Context().document.Channels().Find(layer.id, property) !=
               nullptr;
      });
  return !has_channels && layer.transform == LayerTransform{};
}

void TransformLayerTool::Press(const ToolInput& input) {
  const Document& document = Context().document;
  const Layer* layer =
      document.Layers().Find(Context().selection.ActiveLayer());
  if (layer == nullptr) {
    Context().hint(QStringLiteral("Pick a layer to transform"));
    return;
  }
  const bool is_hidden_or_locked = !layer->visible || layer->locked;
  if (is_hidden_or_locked) {
    Context().hint(QStringLiteral("%1 is %2: %3 it to transform it")
                       .arg(layer->name,
                            layer->locked ? QStringLiteral("locked")
                                          : QStringLiteral("hidden"),
                            layer->locked ? QStringLiteral("unlock")
                                          : QStringLiteral("show")));
    return;
  }
  const Pose& pose = Context().frames.Current();
  layer_ = layer->id;
  press_canvas_ = input.canvas;
  start_ = pose.TransformOf(layer_);
  has_moved_ = false;
  drag_tag_ = MovePointsCommand::NewDragTag();
  mode_ = Mode::kMove;

  const std::optional<QRectF> box = ContentBox();
  new_origin_.reset();
  origin_ = layer->origin;
  const bool needs_origin = box.has_value() && IsUntouched(*layer);
  if (needs_origin) {
    new_origin_ = box->center();
    origin_ = box->center();
  }
  if (!box.has_value()) {
    return;
  }

  const QTransform to_screen =
      pose.LayerMatrix(layer_) * input.view->ToScreenMatrix();
  const std::array<QPointF, 4> corners = CornersOf(*box);
  const QPolygonF outline = to_screen.map(QPolygonF(*box));
  for (size_t i = 0; i < corners.size(); ++i) {
    const double distance =
        QLineF(to_screen.map(corners[i]), input.screen).length();
    if (distance <= kHandlePx) {
      mode_ = Mode::kScale;
      handle_ = corners[i];
      anchor_ = corners[(i + 2) % corners.size()];
      return;
    }
    const bool is_turn = distance <= kTurnReachPx &&
                         !outline.containsPoint(input.screen, Qt::OddEvenFill);
    if (is_turn) {
      mode_ = Mode::kTurn;
    }
  }
}

void TransformLayerTool::Drag(const ToolInput& input) {
  switch (mode_) {
    case Mode::kMove: {
      QPointF delta = input.canvas - press_canvas_;
      const bool is_locked = input.modifiers.testFlag(Qt::ShiftModifier);
      if (is_locked) {
        const bool is_sideways = std::abs(delta.x()) >= std::abs(delta.y());
        delta = is_sideways ? QPointF(delta.x(), 0) : QPointF(0, delta.y());
      }
      LayerTransform moved = start_;
      moved.position += delta;
      Place(moved);
      break;
    }
    case Mode::kTurn:
      Turn(input);
      break;
    case Mode::kScale:
      Scale(input);
      break;
    case Mode::kIdle:
      break;
  }
}

// Round the origin, which sits at position + origin on the canvas whatever
// the angle.
void TransformLayerTool::Turn(const ToolInput& input) {
  const QPointF pivot = start_.position + origin_;
  const double from = QLineF(pivot, press_canvas_).angle();
  const double to = QLineF(pivot, input.canvas).angle();
  // QLineF angles run counterclockwise in degrees, and the canvas y axis
  // points down.
  const double turn = qDegreesToRadians(from - to);
  LayerTransform turned = start_;
  turned.angle = start_.angle + turn;
  if (input.modifiers.testFlag(Qt::ShiftModifier)) {
    turned.angle = std::round(turned.angle / kSnapAngle) * kSnapAngle;
  }
  Place(turned);
}

// The opposite corner stays where it is on the canvas.
void TransformLayerTool::Scale(const ToolInput& input) {
  const QTransform start_matrix = start_.Matrix(origin_);
  const QPointF anchor_canvas = start_matrix.map(anchor_);
  QTransform unturn;
  unturn.rotateRadians(-start_.angle);
  const QPointF reach = unturn.map(input.canvas - anchor_canvas);
  const QPointF span = handle_ - anchor_;
  LayerTransform scaled = start_;
  if (std::abs(span.x()) > 1e-6) {
    scaled.scale.setX(KeepAwayFromZero(reach.x() / span.x()));
  }
  if (std::abs(span.y()) > 1e-6) {
    scaled.scale.setY(KeepAwayFromZero(reach.y() / span.y()));
  }
  const bool is_even = input.modifiers.testFlag(Qt::ShiftModifier);
  if (is_even) {
    // How far along the corner's own line the pointer went.
    const QPointF along(span.x() * start_.scale.x(),
                        span.y() * start_.scale.y());
    const double length = QPointF::dotProduct(along, along);
    const double factor =
        length <= 0 ? 1.0 : QPointF::dotProduct(reach, along) / length;
    scaled.scale = QPointF(KeepAwayFromZero(start_.scale.x() * factor),
                           KeepAwayFromZero(start_.scale.y() * factor));
  }
  QTransform turn_and_scale;
  turn_and_scale.rotateRadians(start_.angle);
  turn_and_scale.scale(scaled.scale.x(), scaled.scale.y());
  scaled.position =
      anchor_canvas - origin_ - turn_and_scale.map(anchor_ - origin_);
  Place(scaled);
}

void TransformLayerTool::Place(const LayerTransform& transform) {
  has_moved_ = true;
  Context().undo.Push(Context().animation.PlaceLayer(layer_, transform,
                                                     new_origin_, drag_tag_));
}

void TransformLayerTool::Release(const ToolInput& /*input*/) {
  mode_ = Mode::kIdle;
}

// Escape mid-drag puts the layer back.
void TransformLayerTool::Cancel() {
  const bool is_mid_drag = mode_ != Mode::kIdle && has_moved_;
  if (is_mid_drag) {
    Context().undo.Undo();
  }
  mode_ = Mode::kIdle;
}

void TransformLayerTool::DrawOverlay(QPainter& painter,
                                     const ViewTransform& view) const {
  const std::optional<QRectF> box = ContentBox();
  const Layer* layer =
      Context().document.Layers().Find(Context().selection.ActiveLayer());
  if (!box.has_value() || layer == nullptr) {
    return;
  }
  const Theme& theme = Context().theme;
  const QTransform to_screen =
      Context().frames.Current().LayerMatrix(layer->id) * view.ToScreenMatrix();
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(theme.accent, 1.0));
  painter.setBrush(Qt::NoBrush);
  painter.drawPolygon(to_screen.map(QPolygonF(*box)));
  const double half = kHandlePx / 2;
  painter.setBrush(Qt::white);
  for (const QPointF& corner : CornersOf(*box)) {
    const QPointF at = to_screen.map(corner);
    painter.drawRect(
        QRectF(at - QPointF(half, half), QSizeF(kHandlePx, kHandlePx)));
  }
  // The origin it turns round, once it has one.
  const QPointF origin =
      to_screen.map(IsUntouched(*layer) ? box->center() : layer->origin);
  painter.drawLine(origin - QPointF(kOriginPx, 0),
                   origin + QPointF(kOriginPx, 0));
  painter.drawLine(origin - QPointF(0, kOriginPx),
                   origin + QPointF(0, kOriginPx));
  painter.restore();
}

}  // namespace snapper
