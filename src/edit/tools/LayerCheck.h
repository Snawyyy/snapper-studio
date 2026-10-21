#pragma once

namespace snapper {

struct ToolContext;

// True when the active layer takes new drawing: it exists, is a vector
// layer, and is shown and unlocked. Otherwise hints what to do and returns
// false.
bool CanDrawOnActiveLayer(const ToolContext& context);

}  // namespace snapper
