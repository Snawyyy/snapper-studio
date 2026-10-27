#include "eval/Curves.h"

#include <QtMath>

#include <array>
#include <cmath>
#include <limits>

namespace snapper {

namespace {

// A cycle can land on an earlier cycle key, but never more often than
// this.
constexpr int kMaxCycleJumps = 64;

struct CurveNames {
  Curve curve;
  const char* name;
  const char* file_name;
};

constexpr std::array<CurveNames, kCurveCount> kNames = {{
    {Curve::kSmooth, "Smooth", "smooth"},
    {Curve::kLinear, "Linear", "linear"},
    {Curve::kStep, "Step", "step"},
    {Curve::kEaseIn, "Ease In", "ease_in"},
    {Curve::kEaseOut, "Ease Out", "ease_out"},
    {Curve::kEaseInOut, "Ease In and Out", "ease_in_out"},
    {Curve::kNoisy, "Noisy", "noisy"},
    {Curve::kCycle, "Cycle", "cycle"},
    {Curve::kBounce, "Bounce", "bounce"},
    {Curve::kElastic, "Elastic", "elastic"},
}};

// -1 to 1, the same for the same seed and frame.
double Noise(Id seed, int frame, int axis) {
  auto h = static_cast<quint32>(seed) * 73856093U ^
           static_cast<quint32>(frame) * 19349663U ^
           static_cast<quint32>(axis) * 83492791U;
  h ^= h >> 13U;
  h *= 0x5bd1e995U;
  h ^= h >> 15U;
  return (static_cast<double>(h & 0xFFFFU) / 0xFFFF) * 2.0 - 1.0;
}

double BounceOut(double s) {
  constexpr double kN = 7.5625;
  constexpr double kD = 2.75;
  if (s < 1 / kD) {
    return kN * s * s;
  }
  if (s < 2 / kD) {
    s -= 1.5 / kD;
    return kN * s * s + 0.75;
  }
  if (s < 2.5 / kD) {
    s -= 2.25 / kD;
    return kN * s * s + 0.9375;
  }
  s -= 2.625 / kD;
  return kN * s * s + 0.984375;
}

double ElasticOut(double s) {
  const bool is_end = s <= 0 || s >= 1;
  if (is_end) {
    return s <= 0 ? 0 : 1;
  }
  return std::pow(2, -10 * s) * std::sin((s * 10 - 0.75) * (2 * M_PI / 3)) + 1;
}

// A curve through the keys on either side, so motion flows through keys
// instead of stopping at each.
QPointF Smooth(const std::vector<Key>& all, size_t i, double s) {
  const Key& a = all[i];
  const Key& b = all[i + 1];
  const Key& before = i > 0 ? all[i - 1] : a;
  const Key& after = i + 2 < all.size() ? all[i + 2] : b;
  const double span = b.frame - a.frame;
  const auto slope = [](const Key& from, const Key& to) {
    const double frames = to.frame - from.frame;
    return frames <= 0 ? QPointF() : (to.value - from.value) / frames;
  };
  const QPointF m1 = slope(before, b) * span;
  const QPointF m2 = slope(a, after) * span;
  const double s2 = s * s;
  const double s3 = s2 * s;
  return a.value * (2 * s3 - 3 * s2 + 1) + m1 * (s3 - 2 * s2 + s) +
         b.value * (-2 * s3 + 3 * s2) + m2 * (s3 - s2);
}

}  // namespace

double Curves::Ease(Curve curve, double s) {
  switch (curve) {
    case Curve::kStep:
      return 0;
    case Curve::kEaseIn:
      return s * s;
    case Curve::kEaseOut:
      return 1 - (1 - s) * (1 - s);
    case Curve::kEaseInOut:
      return s * s * (3 - 2 * s);
    case Curve::kBounce:
      return BounceOut(s);
    case Curve::kElastic:
      return ElasticOut(s);
    default:
      return s;
  }
}

QPointF Curves::ValueAt(const std::vector<Key>& keys, const QPointF& setup,
                        int frame, Id seed) {
  if (keys.empty()) {
    return setup;
  }
  std::vector<Key> all;
  all.reserve(keys.size() + 1);
  all.push_back({.frame = 0, .value = setup});
  all.insert(all.end(), keys.begin(), keys.end());

  size_t i = 0;
  for (int jump = 0; jump <= kMaxCycleJumps; ++jump) {
    i = 0;
    while (i + 1 < all.size() && all[i + 1].frame <= frame) {
      ++i;
    }
    const Key& key = all[i];
    const int next =
        i + 1 < all.size() ? all[i + 1].frame : std::numeric_limits<int>::max();
    const bool is_cycling = key.curve == Curve::kCycle && i > 0 &&
                            frame > key.frame && frame < next;
    if (!is_cycling) {
      break;
    }
    const int back = key.param > 0 ? static_cast<int>(key.param)
                                   : key.frame - all[i - 1].frame;
    frame = std::max(0, key.frame - back + (frame - key.frame) % back);
  }

  if (i + 1 >= all.size()) {
    return all[i].value;
  }
  const Key& a = all[i];
  const Key& b = all[i + 1];
  const double s = static_cast<double>(frame - a.frame) / (b.frame - a.frame);
  switch (a.curve) {
    case Curve::kSmooth:
      return Smooth(all, i, s);
    case Curve::kNoisy: {
      const double fade = std::sin(M_PI * s) * a.param;
      return a.value + (b.value - a.value) * s +
             QPointF(Noise(seed, frame, 0), Noise(seed, frame, 1)) * fade;
    }
    default:
      return a.value + (b.value - a.value) * Ease(a.curve, s);
  }
}

QString Curves::Name(Curve curve) {
  return QString::fromLatin1(kNames[static_cast<size_t>(curve)].name);
}

QString Curves::FileName(Curve curve) {
  return QString::fromLatin1(kNames[static_cast<size_t>(curve)].file_name);
}

Curve Curves::FromFileName(const QString& name) {
  for (const CurveNames& entry : kNames) {
    if (name == QLatin1String(entry.file_name)) {
      return entry.curve;
    }
  }
  return Curve::kSmooth;
}

}  // namespace snapper
