#include "app/ThemeManager.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>

#include <array>

namespace snapper {

namespace {

struct ColorField {
  const char* key;
  QColor Theme::* member;
};

struct SizeField {
  const char* key;
  int Theme::* member;
};

constexpr std::array<ColorField, 15> kColorFields = {{
    {"window", &Theme::window},
    {"raised", &Theme::raised},
    {"hover", &Theme::hover},
    {"pressed", &Theme::pressed},
    {"canvas_surround", &Theme::canvas_surround},
    {"separator", &Theme::separator},
    {"text", &Theme::text},
    {"text_secondary", &Theme::text_secondary},
    {"text_disabled", &Theme::text_disabled},
    {"accent", &Theme::accent},
    {"accent_pill", &Theme::accent_pill},
    {"setup_mode", &Theme::setup_mode},
    {"action_mode", &Theme::action_mode},
    {"warning", &Theme::warning},
    {"error", &Theme::error},
}};

constexpr std::array<SizeField, 9> kSizeFields = {{
    {"radius_control", &Theme::radius_control},
    {"radius_pill", &Theme::radius_pill},
    {"radius_popup", &Theme::radius_popup},
    {"panel_padding", &Theme::panel_padding},
    {"row_height", &Theme::row_height},
    {"button_size", &Theme::button_size},
    {"icon_size", &Theme::icon_size},
    {"font_body", &Theme::font_body},
    {"font_small", &Theme::font_small},
}};

// Style sheets want rgba() so the alpha survives.
QString Css(const QColor& color) {
  return QStringLiteral("rgba(%1, %2, %3, %4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(color.alpha());
}

}  // namespace

bool ThemeManager::LoadFromFile(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    error_ = QStringLiteral("Can't open theme file %1").arg(path);
    return false;
  }
  return LoadFromJson(file.readAll());
}

// Parses into a copy so a bad file leaves the current theme untouched.
bool ThemeManager::LoadFromJson(const QByteArray& json) {
  QJsonParseError parse_error;
  const QJsonDocument doc = QJsonDocument::fromJson(json, &parse_error);
  const bool is_object =
      parse_error.error == QJsonParseError::NoError && doc.isObject();
  if (!is_object) {
    error_ = QStringLiteral("Theme file is not a JSON object: %1")
                 .arg(parse_error.errorString());
    return false;
  }

  const QJsonObject root = doc.object();
  const QJsonObject colors = root.value(QStringLiteral("colors")).toObject();
  const QJsonObject sizes = root.value(QStringLiteral("sizes")).toObject();
  Theme loaded;

  for (const ColorField& field : kColorFields) {
    const QString key = QString::fromLatin1(field.key);
    const QColor color = QColor::fromString(colors.value(key).toString());
    if (!color.isValid()) {
      error_ = QStringLiteral("Theme color \"%1\" is missing or bad").arg(key);
      return false;
    }
    loaded.*field.member = color;
  }

  for (const SizeField& field : kSizeFields) {
    const QString key = QString::fromLatin1(field.key);
    const int size = sizes.value(key).toInt(-1);
    if (size < 0) {
      error_ = QStringLiteral("Theme size \"%1\" is missing or bad").arg(key);
      return false;
    }
    loaded.*field.member = size;
  }

  loaded.font_family = root.value(QStringLiteral("font_family")).toString();
  theme_ = std::move(loaded);
  error_.clear();
  return true;
}

void ThemeManager::ApplyTo(QApplication& app) const {
  QApplication::setStyle(QStringLiteral("Fusion"));
  QApplication::setPalette(BuildPalette());
  // Until the font ships with the app, a missing one keeps the system font.
  QFont font = QApplication::font();
  const bool has_theme_font = QFontDatabase::hasFamily(theme_.font_family);
  if (has_theme_font) {
    font.setFamily(theme_.font_family);
  }
  font.setPixelSize(theme_.font_body);
  QApplication::setFont(font);
  app.setStyleSheet(BuildStyleSheet());
}

QPalette ThemeManager::BuildPalette() const {
  QPalette palette;
  palette.setColor(QPalette::Window, theme_.window);
  palette.setColor(QPalette::Base, theme_.raised);
  palette.setColor(QPalette::AlternateBase, theme_.window);
  palette.setColor(QPalette::Button, theme_.raised);
  palette.setColor(QPalette::Mid, theme_.hover);
  palette.setColor(QPalette::Dark, theme_.canvas_surround);
  palette.setColor(QPalette::Light, theme_.hover);
  palette.setColor(QPalette::WindowText, theme_.text);
  palette.setColor(QPalette::Text, theme_.text);
  palette.setColor(QPalette::ButtonText, theme_.text);
  palette.setColor(QPalette::PlaceholderText, theme_.text_secondary);
  palette.setColor(QPalette::ToolTipBase, theme_.raised);
  palette.setColor(QPalette::ToolTipText, theme_.text);
  palette.setColor(QPalette::Highlight, theme_.accent);
  palette.setColor(QPalette::HighlightedText, theme_.text);
  palette.setColor(QPalette::Accent, theme_.accent);
  palette.setColor(QPalette::Disabled, QPalette::WindowText,
                   theme_.text_disabled);
  palette.setColor(QPalette::Disabled, QPalette::Text, theme_.text_disabled);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                   theme_.text_disabled);
  return palette;
}

QString ThemeManager::BuildStyleSheet() const {
  const Theme& t = theme_;
  QString css;
  css += QStringLiteral("#canvas { background: %1; }\n")
             .arg(Css(t.canvas_surround));
  css += QStringLiteral(
             "#command-box { background: %1; border: 1px solid %2;"
             " border-radius: %3px; }\n")
             .arg(Css(t.raised), Css(t.separator))
             .arg(t.radius_popup);
  css += QStringLiteral(
             "QTabWidget::pane { background: %1; border: none;"
             " border-top: 1px solid %2; }\n")
             .arg(Css(t.window), Css(t.separator));
  css += QStringLiteral(
             "QToolTip { background: %1; color: %2; border: 1px solid %3;"
             " border-radius: %4px; padding: 4px 8px; }\n")
             .arg(Css(t.raised), Css(t.text), Css(t.separator))
             .arg(t.radius_control);
  css += QStringLiteral(
             "QLineEdit { background: %1; color: %2; border: 1px solid %3;"
             " border-radius: %4px; padding: 4px 8px;"
             " selection-background-color: %5; }\n"
             "QLineEdit:focus { border-color: %6; }\n")
             .arg(Css(t.raised), Css(t.text), Css(t.separator))
             .arg(t.radius_control)
             .arg(Css(t.accent_pill), Css(t.accent));
  css += QStringLiteral(
             "QListView { background: transparent; border: none;"
             " outline: none; }\n"
             "QListView::item { min-height: %1px; padding: 0 8px;"
             " border-radius: %2px; color: %3; }\n"
             "QListView::item:hover { background: %4; }\n"
             "QListView::item:selected { background: %5; color: %3; }\n")
             .arg(t.row_height)
             .arg(t.radius_pill)
             .arg(Css(t.text), Css(t.hover), Css(t.accent_pill));
  css += QStringLiteral(
             "QTabBar::tab { background: transparent; color: %1;"
             " padding: 6px 12px; border: none; border-radius: %2px; }\n"
             "QTabBar::tab:selected { color: %3; background: %4; }\n"
             "QTabBar::tab:hover:!selected { background: %4; }\n")
             .arg(Css(t.text_secondary))
             .arg(t.radius_control)
             .arg(Css(t.text), Css(t.hover));
  css += QStringLiteral(
             "QScrollBar { background: transparent; width: 10px;"
             " height: 10px; }\n"
             "QScrollBar::handle { background: %1; border-radius: 5px; }\n"
             "QScrollBar::add-line, QScrollBar::sub-line { width: 0;"
             " height: 0; }\n")
             .arg(Css(t.hover));
  return css;
}

}  // namespace snapper
