#include "document/StandardBrushes.h"

#include <QBuffer>
#include <QImage>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>

namespace snapper {

namespace {

constexpr int kTipSize = 64;
constexpr double kTipCenter = (kTipSize - 1) / 2.0;

// A repeatable random number from 0 to 1 for a spot and a seed.
double Noise(int x, int y, std::uint32_t seed) {
  std::uint32_t h = static_cast<std::uint32_t>(x) * 374761393U +
                    static_cast<std::uint32_t>(y) * 668265263U + seed * 69069U;
  h = (h ^ (h >> 13U)) * 1274126177U;
  h ^= h >> 16U;
  return static_cast<double>(h & 0xFFFFU) / 0xFFFF;
}

// 1 inside the edge, 0 outside, with a soft step `soft` pixels wide.
double Edge(double inside_by, double soft) {
  return std::clamp(inside_by / soft + 0.5, 0.0, 1.0);
}

// A white picture whose alpha at each pixel comes from `coverage`, given
// the pixel's offset from the middle.
QByteArray Draw(
    const std::function<double(int, int, double, double)>& coverage) {
  QImage image(kTipSize, kTipSize, QImage::Format_ARGB32);
  for (int y = 0; y < kTipSize; ++y) {
    for (int x = 0; x < kTipSize; ++x) {
      const double alpha =
          std::clamp(coverage(x, y, x - kTipCenter, y - kTipCenter), 0.0, 1.0);
      image.setPixel(
          x, y,
          qRgba(255, 255, 255, static_cast<int>(std::lround(alpha * 255))));
    }
  }
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  return bytes;
}

// Grainy and soft, a little lighter than full paint.
QByteArray PencilTip() {
  return Draw([](int x, int y, double dx, double dy) {
    const double r = std::hypot(dx, dy);
    const double body = Edge(26.0 - r, 8.0);
    const double grain = Noise(x / 2, y / 2, 1) > 0.45 ? 1.0 : 0.1;
    return body * grain * 0.4;
  });
}

// A hard oval, so an aligned line swells a little on curves.
QByteArray InkTip() {
  return Draw([](int /*x*/, int /*y*/, double dx, double dy) {
    const double r = std::hypot(dx / 30.0, dy / 22.0);
    return Edge((1.0 - r) * 26.0, 1.5);
  });
}

// Scattered specks inside a round body.
QByteArray ChalkTip() {
  return Draw([](int x, int y, double dx, double dy) {
    const double r = std::hypot(dx, dy);
    const double body = Edge(29.0 - r, 10.0);
    const double speck = Noise(x / 2, y / 2, 7) > 0.55 ? 0.95 : 0.0;
    return body * speck;
  });
}

// A flat, see-through chisel, short along the line and wide across it.
QByteArray MarkerTip() {
  return Draw([](int /*x*/, int /*y*/, double dx, double dy) {
    const double along = Edge(11.0 - std::abs(dx), 3.0);
    const double across = Edge(30.0 - std::abs(dy), 3.0);
    return along * across * 0.35;
  });
}

}  // namespace

std::vector<StandardBrush> StandardBrushes() {
  return {
      {.brush = {.name = QStringLiteral("Pencil"),
                 .spacing = 0.25F,
                 .align = false,
                 .angle_jitter = 180.0F,
                 .size_jitter = 0.1F,
                 .boil_every = 2},
       .png = PencilTip()},
      {.brush = {.name = QStringLiteral("Ink"),
                 .spacing = 0.1F,
                 .align = true,
                 .size_jitter = 0.05F},
       .png = InkTip()},
      {.brush = {.name = QStringLiteral("Chalk"),
                 .spacing = 0.3F,
                 .align = false,
                 .angle_jitter = 180.0F,
                 .spacing_jitter = 0.3F,
                 .size_jitter = 0.2F,
                 .boil_every = 2},
       .png = ChalkTip()},
      {.brush = {.name = QStringLiteral("Marker"),
                 .spacing = 0.08F,
                 .align = true},
       .png = MarkerTip()},
  };
}

}  // namespace snapper
