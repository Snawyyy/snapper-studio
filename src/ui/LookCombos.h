#pragma once

#include "document/Ids.h"

class QComboBox;

namespace snapper {

class BrushManager;
class StyleManager;

// Fills a combo box with every brush, after "Plain line", and picks
// `current`. Each item holds its brush id, kNoId for a plain line.
void FillBrushCombo(QComboBox& combo, const BrushManager& brushes, Id current);
// Fills a combo box with every style, after "None", and picks `current`.
void FillStyleCombo(QComboBox& combo, const StyleManager& styles, Id current);
// The id held by the picked item.
Id PickedId(const QComboBox& combo);

}  // namespace snapper
