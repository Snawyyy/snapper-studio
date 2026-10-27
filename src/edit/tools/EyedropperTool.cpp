#include "edit/tools/EyedropperTool.h"

#include <QPainter>

#include <cmath>

#include "document/Document.h"
#include "edit/HitTest.h"
#include "edit/LookManager.h"
#include "eval/FrameEvaluator.h"
#include "render/ViewTransform.h"

namespace snapper {

namespace {

constexpr double kSampleRadius = 10.0;
constexpr double kSampleOffset = 22.0;

// The picture's pixel under a point in layer space, if the layer shows
// one there.
std::optional<QColor> PixelAt(const Document& document, const Layer& layer,
                              const QPointF& spot) {
  const QImage* image = document.Images().Find(layer.image);
  const bool has_picture = image != nullptr && layer.image_scale > 0;
  if (!has_picture) {
    return std::nullopt;
  }
  const QPointF at = (spot - layer.image_pos) / layer.image_scale;
  const QPoint pixel(static_cast<int>(std::floor(at.x())),
                     static_cast<int>(std::floor(at.y())));
  const bool is_inside = image->rect().contains(pixel);
  if (!is_inside) {
    return std::nullopt;
  }
  const QColor color = image->pixelColor(pixel);
  return color.alpha() == 0 ? std::nullopt : std::optional<QColor>(color);
}

}  // namespace

EyedropperTool::EyedropperTool(ToolContext context)
    : Tool(std::move(context)) {}

ToolInfo EyedropperTool::Info() const {
  return {.id = QStringLiteral("eyedropper"),
          .title = QStringLiteral("Eyedropper"),
          .key = QKeySequence(Qt::Key_I),
          .hint = QStringLiteral(
              "Eyedropper: click a line, fill or picture to pick its color. "
              "Alt+click picks a shape's whole style.")};
}

std::optional<EyedropperTool::Pick> EyedropperTool::PickAt(
    const ToolInput& input) const {
  const Document& document = Context().document;
  if (input.view == nullptr) {
    return std::nullopt;
  }
  const ViewTransform& view = *input.view;
  const Pose& pose = Context().frames.Current();
  const std::vector<Layer>& layers = document.Layers().All();
  for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
    if (!document.Layers().IsShown(it->id)) {
      continue;
    }
    if (it->type == LayerType::kImage) {
      const std::optional<QColor> pixel = PixelAt(
          document, *it, pose.LayerMatrix(it->id).inverted().map(input.canvas));
      if (pixel.has_value()) {
        return Pick{.kind = Pick::Kind::kPicture,
                    .color = *pixel,
                    .shape = std::nullopt};
      }
      continue;
    }
    const Id id = HitTest::ShapeAt(document, pose, it->id, view, input.screen);
    const Shape* shape = pose.FindShape(id);
    if (shape == nullptr) {
      continue;
    }
    const ShapeLook look =
        document.Styles().Resolve(shape->look, shape->styles);
    const bool is_line =
        look.has_line &&
        HitTest::IsOnLine(document, pose, *shape, view, input.screen);
    return Pick{.kind = is_line ? Pick::Kind::kLine : Pick::Kind::kFill,
                .color = is_line ? look.line : look.fill,
                .shape = ShapeLookState{
                    .shape = id, .look = shape->look, .styles = shape->styles}};
  }
  return std::nullopt;
}

void EyedropperTool::Press(const ToolInput& input) {
  const std::optional<Pick> pick = PickAt(input);
  if (!pick.has_value()) {
    Context().hint(QStringLiteral("Eyedropper: nothing to pick here"));
    return;
  }
  const bool is_flipped = input.modifiers.testFlag(Qt::AltModifier);
  const Mode other = mode_ == Mode::kColor ? Mode::kWholeStyle : Mode::kColor;
  Apply(*pick, is_flipped ? other : mode_);
}

void EyedropperTool::Apply(const Pick& pick, Mode mode) {
  LookManager& looks = Context().looks;
  const QString target = looks.HasSelectedShapes()
                             ? QStringLiteral("the selected shapes")
                             : QStringLiteral("new shapes");
  if (mode == Mode::kWholeStyle) {
    if (!pick.shape.has_value()) {
      Context().hint(
          QStringLiteral("Eyedropper: a picture has no style, pick a shape"));
      return;
    }
    looks.ChangeAll(QStringLiteral("Pick Style"), pick.shape->look,
                    pick.shape->styles);
    Context().hint(QStringLiteral("Picked the whole style for %1").arg(target));
    return;
  }
  const bool is_fill = pick.kind == Pick::Kind::kFill;
  const LookPart part = is_fill ? LookPart::kFill : LookPart::kLine;
  const QString style = looks.StyleSetting(part);
  if (!style.isEmpty()) {
    Context().hint(
        QStringLiteral("The %1 color comes from the style %2: edit the style, "
                       "or Alt+click to pick the whole style")
            .arg(is_fill ? QStringLiteral("fill") : QStringLiteral("line"),
                 style));
    return;
  }
  const QColor color = pick.color;
  looks.Change(is_fill ? QStringLiteral("Pick Fill Color")
                       : QStringLiteral("Pick Line Color"),
               [&](ShapeLookState& state) {
                 if (is_fill) {
                   state.look.has_fill = true;
                   state.look.fill = color;
                 } else {
                   state.look.has_line = true;
                   state.look.line = color;
                 }
               });
  Context().hint(
      QStringLiteral("Picked %1 as the %2 color of %3")
          .arg(color.name().toUpper(),
               is_fill ? QStringLiteral("fill") : QStringLiteral("line"),
               target));
}

void EyedropperTool::Hover(const ToolInput& input) {
  hover_ = PickAt(input);
  hover_screen_ = input.screen;
}

// A drop of the color it would pick, beside the pointer.
void EyedropperTool::DrawOverlay(QPainter& painter,
                                 const ViewTransform& /*view*/) const {
  if (!hover_.has_value()) {
    return;
  }
  const QPointF center = hover_screen_ + QPointF(kSampleOffset, -kSampleOffset);
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(QColor(0, 0, 0, 140), 3.0));
  painter.setBrush(hover_->color);
  painter.drawEllipse(center, kSampleRadius, kSampleRadius);
  painter.setPen(QPen(Qt::white, 1.5));
  painter.setBrush(Qt::NoBrush);
  painter.drawEllipse(center, kSampleRadius, kSampleRadius);
  painter.restore();
}

}  // namespace snapper
