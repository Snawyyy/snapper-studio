#include "ui/LookCombos.h"

#include <QComboBox>
#include <QSignalBlocker>

#include "document/BrushManager.h"
#include "document/StyleManager.h"

namespace snapper {

namespace {

void PickId(QComboBox& combo, Id current) {
  const int index = combo.findData(current);
  combo.setCurrentIndex(index < 0 ? 0 : index);
}

}  // namespace

void FillBrushCombo(QComboBox& combo, const BrushManager& brushes, Id current) {
  const QSignalBlocker blocker(&combo);
  combo.clear();
  combo.addItem(QStringLiteral("Plain line"), kNoId);
  for (const Brush& brush : brushes.All()) {
    combo.addItem(brush.name, brush.id);
  }
  PickId(combo, current);
}

void FillStyleCombo(QComboBox& combo, const StyleManager& styles, Id current) {
  const QSignalBlocker blocker(&combo);
  combo.clear();
  combo.addItem(QStringLiteral("None"), kNoId);
  for (const Style& style : styles.All()) {
    combo.addItem(style.name, style.id);
  }
  PickId(combo, current);
}

Id PickedId(const QComboBox& combo) { return combo.currentData().toInt(); }

}  // namespace snapper
