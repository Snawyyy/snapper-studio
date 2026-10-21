#pragma once

#include <QString>

namespace snapper {

struct ToolContext;

// True when the active layer can be changed: it exists, is a vector layer,
// and is shown and unlocked. Otherwise hints what to do and returns false.
// `action` finishes the hint: "draw on", "edit".
bool CanChangeActiveLayer(const ToolContext& context, const QString& action);

}  // namespace snapper
