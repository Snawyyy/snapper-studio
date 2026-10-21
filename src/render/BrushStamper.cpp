#include "render/BrushStamper.h"

#include <QLineF>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

#include "render/ShapeGeometry.h"

namespace snapper {

namespace {

// Largest tinted picture, in screen pixels across.
constexpr int kMaxTintSide = 512;

// A small, fast random source that repeats for the same seed.
class Random {
 public:
  explicit Random(std::uint64_t seed) : state_(seed) {}

  // -1 to 1.
  double Spread() { return Next() * 2.0 - 1.0; }

 private:
  // 0 to 1, by splitmix64.
  double Next() {
    state_ += 0x9E3779B97F4A7C15ULL;
    std::uint64_t z = state_;
    z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
    z ^= z >> 31U;
    return static_cast<double>(z >> 11U) / static_cast<double>(1ULL << 53U);
  }

  std::uint64_t state_;
};

std::uint64_t SeedFor(Id shape, const Brush& brush, int frame) {
  const int boil = brush.boil_every > 0 ? frame / brush.boil_every : 0;
  return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(shape))
          << 32U) ^
         static_cast<std::uint32_t>(boil);
}

double DegreesAlong(const QPointF& along) {
  return std::atan2(along.y(), along.x()) * 180.0 / std::numbers::pi;
}

}  // namespace

std::vector<BrushStamp> BrushStamper::Plan(const Shape& shape,
                                           double line_width,
                                           const Brush& brush, int frame) {
  std::vector<BrushStamp> stamps;
  const std::vector<LineSample> samples = ShapeGeometry::Walk(shape);
  if (samples.empty() || line_width <= 0) {
    return stamps;
  }
  Random random(SeedFor(shape.id, brush, frame));
  const auto place = [&](const QPointF& pos, double width,
                         const QPointF& along) {
    const double size =
        line_width * width * (1.0 + brush.size_jitter * random.Spread());
    const double turn = brush.align ? DegreesAlong(along) : 0.0;
    stamps.push_back(
        {.pos = pos,
         .size = std::max(0.0, size),
         .angle_degrees = turn + brush.angle_jitter * random.Spread()});
  };
  const auto next_gap = [&](double width) {
    const double gap = brush.spacing * line_width * width *
                       (1.0 + brush.spacing_jitter * random.Spread());
    return std::max(gap, kMinGap);
  };

  const QPointF first_along =
      samples.size() > 1 ? samples[1].pos - samples[0].pos : QPointF(1, 0);
  place(samples[0].pos, samples[0].width, first_along);
  // How far along the line the next stamp is, from where the walk stands.
  double to_next = next_gap(samples[0].width);
  for (size_t i = 1; i < samples.size(); ++i) {
    const LineSample& from = samples[i - 1];
    const LineSample& to = samples[i];
    const QPointF along = to.pos - from.pos;
    const double length = std::hypot(along.x(), along.y());
    double at = to_next;
    while (at <= length) {
      const bool is_full =
          static_cast<int>(stamps.size()) >= kMaxStampsPerShape;
      if (is_full) {
        return stamps;
      }
      const double t = at / length;
      const double width = from.width + (to.width - from.width) * t;
      place(from.pos + along * t, width, along);
      at += next_gap(width);
    }
    to_next = at - length;
  }
  return stamps;
}

QImage BrushStamper::Tint(const QImage& picture, const QColor& color,
                          int side) {
  const int width = std::clamp(side, 1, kMaxTintSide);
  const int height =
      std::clamp(static_cast<int>(
                     std::lround(static_cast<double>(width) * picture.height() /
                                 std::max(1, picture.width()))),
                 1, kMaxTintSide);
  const QImage scaled = picture
                            .scaled(width, height, Qt::IgnoreAspectRatio,
                                    Qt::SmoothTransformation)
                            .convertToFormat(QImage::Format_ARGB32);
  const bool counts_dark = !picture.hasAlphaChannel();
  QImage tinted(scaled.size(), QImage::Format_ARGB32_Premultiplied);
  const QRgb paint = color.rgba();
  for (int y = 0; y < scaled.height(); ++y) {
    const auto* in = reinterpret_cast<const QRgb*>(scaled.constScanLine(y));
    auto* out = reinterpret_cast<QRgb*>(tinted.scanLine(y));
    for (int x = 0; x < scaled.width(); ++x) {
      const int coverage = counts_dark ? 255 - qGray(in[x]) : qAlpha(in[x]);
      const int alpha = qAlpha(paint) * coverage / 255;
      out[x] =
          qPremultiply(qRgba(qRed(paint), qGreen(paint), qBlue(paint), alpha));
    }
  }
  return tinted;
}

void BrushStamper::Draw(QPainter& painter, const Shape& shape,
                        double line_width, const QColor& color,
                        const Brush& brush, const QImage& picture, int frame) {
  const std::vector<BrushStamp> stamps = Plan(shape, line_width, brush, frame);
  if (stamps.empty() || picture.isNull()) {
    return;
  }
  const QTransform base = painter.worldTransform();
  // Tinted once at the size the biggest stamp has on screen, so small
  // lines stay crisp instead of shrinking a big picture each stamp.
  const double screen_scale = std::sqrt(std::abs(base.determinant()));
  const double biggest = std::ranges::max(stamps, {}, &BrushStamp::size).size;
  const QImage tinted =
      Tint(picture, color, static_cast<int>(std::ceil(biggest * screen_scale)));
  const double aspect = static_cast<double>(tinted.height()) / tinted.width();

  painter.save();
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  for (const BrushStamp& stamp : stamps) {
    QTransform place;
    place.translate(stamp.pos.x(), stamp.pos.y());
    place.rotate(stamp.angle_degrees);
    painter.setWorldTransform(place * base);
    const double height = stamp.size * aspect;
    painter.drawImage(QRectF(-stamp.size / 2, -height / 2, stamp.size, height),
                      tinted);
  }
  painter.restore();
}

}  // namespace snapper
