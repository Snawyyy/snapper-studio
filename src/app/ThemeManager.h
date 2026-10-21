#pragma once

#include <QPalette>
#include <QString>

#include "app/Theme.h"

class QApplication;

namespace snapper {

// Owns the theme: reads the theme file and turns it into the palette and
// style sheet the app runs with. Nothing else hardcodes a color or size.
class ThemeManager {
 public:
  // Reads a theme file. On failure the current theme is kept and Error()
  // says what was wrong.
  bool LoadFromFile(const QString& path);
  bool LoadFromJson(const QByteArray& json);

  void ApplyTo(QApplication& app) const;

  const Theme& CurrentTheme() const { return theme_; }
  const QString& Error() const { return error_; }

  QPalette BuildPalette() const;
  QString BuildStyleSheet() const;

 private:
  Theme theme_;
  QString error_;
};

}  // namespace snapper
