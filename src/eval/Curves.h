#pragma once

#include <QPointF>
#include <QString>

#include <vector>

#include "document/ChannelManager.h"

namespace snapper {

// The math of key curves: what a channel's value is on any frame.
class Curves {
 public:
  // The value on the frame. `setup` is the value on frame 0, which counts
  // as a smooth key there. `seed` keeps noise the same every time.
  static QPointF ValueAt(const std::vector<Key>& keys, const QPointF& setup,
                         int frame, Id seed);

  // How far along a span the value is, from 0 at its first key to 1 at
  // the next, for curves that only reshape time.
  static double Ease(Curve curve, double s);

  static QString Name(Curve curve);
  // The name used in project files.
  static QString FileName(Curve curve);
  // kSmooth for a name it doesn't know.
  static Curve FromFileName(const QString& name);
};

}  // namespace snapper
