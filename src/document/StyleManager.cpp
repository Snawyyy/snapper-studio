#include "document/StyleManager.h"

#include <algorithm>

namespace snapper {

StyleManager::StyleManager(QObject* parent) : QObject(parent) {}

bool StyleManager::Insert(const Style& style, int index) {
  const bool is_full = Count() >= kMaxStyles;
  const bool is_bad_id = style.id == kNoId || IndexOf(style.id) >= 0;
  if (is_full || is_bad_id) {
    return false;
  }
  const int at = std::clamp(index, 0, Count());
  styles_.insert(styles_.begin() + at, style);
  Q_EMIT ListChanged();
  return true;
}

std::optional<RemovedStyle> StyleManager::Take(Id id) {
  const int index = IndexOf(id);
  if (index < 0) {
    return std::nullopt;
  }
  RemovedStyle removed{.style = styles_[static_cast<size_t>(index)],
                       .index = index};
  styles_.erase(styles_.begin() + index);
  Q_EMIT ListChanged();
  return removed;
}

bool StyleManager::Set(const Style& style) {
  const int index = IndexOf(style.id);
  if (index < 0) {
    return false;
  }
  styles_[static_cast<size_t>(index)] = style;
  Q_EMIT StyleChanged(style.id);
  return true;
}

void StyleManager::Clear() {
  styles_.clear();
  Q_EMIT ListChanged();
}

const Style* StyleManager::Find(Id id) const {
  const int index = IndexOf(id);
  return index < 0 ? nullptr : &styles_[static_cast<size_t>(index)];
}

int StyleManager::IndexOf(Id id) const {
  if (id == kNoId) {
    return -1;
  }
  const auto found = std::ranges::find(styles_, id, &Style::id);
  return found == styles_.end()
             ? -1
             : static_cast<int>(std::distance(styles_.begin(), found));
}

ShapeLook StyleManager::Resolve(const ShapeLook& own,
                                const StylePair& styles) const {
  ShapeLook look = own;
  for (const Id id : styles) {
    const Style* style = Find(id);
    if (style == nullptr) {
      continue;
    }
    const StyleParts& parts = style->parts;
    if (parts.fill) {
      look.has_fill = style->look.has_fill;
      look.fill = style->look.fill;
    }
    if (parts.line) {
      look.has_line = style->look.has_line;
      look.line = style->look.line;
    }
    if (parts.width) {
      look.line_width = style->look.line_width;
    }
    if (parts.brush) {
      look.brush = style->look.brush;
    }
  }
  return look;
}

}  // namespace snapper
