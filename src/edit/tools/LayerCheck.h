#pragma once

#include <QString>

namespace snapper {

struct Shape;
struct ToolContext;

// True when the active layer can be changed: it exists, is a vector layer,
// and is shown and unlocked. Otherwise hints what to do and returns false.
// `action` finishes the hint: "draw on", "edit".
bool CanChangeActiveLayer(const ToolContext& context, const QString& action);

// Moves a shape drawn on the canvas into its layer's own space, so it
// lands where it was drawn however the layer is moved, turned or scaled on
// this frame.
void MoveIntoLayer(const ToolContext& context, Shape& shape);

}  // namespace snapper
