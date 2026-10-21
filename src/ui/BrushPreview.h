#pragma once

#include <QColor>
#include <QImage>
#include <QSize>

#include "document/Ids.h"

namespace snapper {

class Document;

// A sample stroke drawn with a brush, swelling from thin to full width and
// back, for brush lists and the brush editor. kNoId draws a plain line.
QImage BrushPreview(const Document& document, Id brush, const QSize& size,
                    const QColor& color, qreal pixel_ratio);

}  // namespace snapper
