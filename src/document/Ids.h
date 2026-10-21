#pragma once

#include <cstdint>

namespace snapper {

// Every object in a document is named by an id that is unique in the
// project. Links between objects use ids, never pointers.
using Id = std::int32_t;
constexpr Id kNoId = 0;

}  // namespace snapper
