#pragma once

#include <QByteArray>

#include <vector>

#include "document/BrushManager.h"

namespace snapper {

// A brush that comes with the app: its settings, without an id or picture
// name yet, and its picture as PNG bytes.
struct StandardBrush {
  Brush brush;
  QByteArray png;
};

// The brushes every new project starts with: pencil, ink, chalk and
// marker. Their pictures are drawn here, the same every time, so a
// project stores each only once.
std::vector<StandardBrush> StandardBrushes();

}  // namespace snapper
