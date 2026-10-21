#pragma once

#include <QString>
#include <QStringList>

namespace snapper {

// Collects what loading a project dropped, keeping only the first few so
// a badly broken file can't flood the message.
class JsonWarnings {
 public:
  static constexpr int kMaxWarnings = 20;

  void Add(const QString& text) {
    ++count_;
    if (list_.size() < kMaxWarnings) {
      list_.append(text);
    }
  }
  QStringList Take() {
    const int hidden = count_ - static_cast<int>(list_.size());
    if (hidden > 0) {
      list_.append(QStringLiteral("and %1 more").arg(hidden));
    }
    return std::move(list_);
  }

 private:
  QStringList list_;
  int count_ = 0;
};

}  // namespace snapper
