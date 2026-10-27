#pragma once

#include <QPointF>
#include <QString>

#include "document/Ids.h"

namespace snapper {

struct Shape;
struct ToolContext;

// True when the active layer can be changed: it exists, is a vector layer,
// and is shown and unlocked. Otherwise hints what to do and returns false.
// `action` finishes the hint: "draw on", "edit".
bool CanChangeActiveLayer(const ToolContext& context, const QString& action);

// The bone layer the bone tools work on, when it exists, is shown and
// unlocked. Otherwise hints what to do and returns kNoId. `action`
// finishes the hint: "add bones to".
Id BoneLayerToEdit(const ToolContext& context, const QString& action);

// A canvas spot in the bone layer's space, as posed now.
QPointF InBoneLayer(const ToolContext& context, Id bone_layer,
                    const QPointF& canvas);

// Moves a shape drawn on the canvas into its layer's own space, so it
// lands where it was drawn however the layer is moved, turned or scaled on
// this frame.
void MoveIntoLayer(const ToolContext& context, Shape& shape);

}  // namespace snapper
