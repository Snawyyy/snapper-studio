#pragma once

#include <QColor>
#include <QRect>
#include <QSet>
#include <QString>

#include "document/Ids.h"

class QPainter;

namespace snapper {

class Document;
struct Pose;
struct Theme;
class ViewTransform;

// Draws the editing aids over the artwork, in screen coordinates.
class OverlayRenderer {
 public:
  // Fades artwork that lies off the page and draws a soft shadow round
  // the page. `cover` is the color around the canvas.
  static void DrawSurround(QPainter& painter, const Document& document,
                           const ViewTransform& view, const QRect& area,
                           const QColor& cover);
  // Outlines every shape using the style, so you see what editing it
  // changes.
  static void DrawStyleUsers(QPainter& painter, const Document& document,
                             const Pose& pose, const ViewTransform& view,
                             Id style, const QColor& color);
  // Draws the bones of a bone layer, Moho style, as kites from base to
  // tip: selected ones in the accent, a ring at the base of pinned ones.
  // Faded ones show the bones while working on a layer inside.
  static void DrawBones(QPainter& painter, const Pose& pose,
                        const ViewTransform& view, Id bone_layer,
                        const QSet<Id>& selected, bool is_faded,
                        const QSet<Id>& pinned, const Theme& theme);
  // A border round the canvas view with a label in its corner, so the
  // mode is never in doubt.
  static void DrawModeBorder(QPainter& painter, const QRect& area,
                             const QColor& color, const QString& label,
                             const Theme& theme);
};

}  // namespace snapper
