#pragma once

#include <QColor>
#include <QString>

namespace snapper {

// Every color and size the UI uses. Colors with transparency are written
// #AARRGGBB in the theme file.
struct Theme {
  QColor window;
  QColor raised;
  QColor hover;
  QColor pressed;
  QColor canvas_surround;
  QColor separator;
  QColor text;
  QColor text_secondary;
  QColor text_disabled;
  QColor accent;
  QColor accent_pill;
  QColor setup_mode;
  QColor action_mode;
  QColor warning;
  QColor error;

  int radius_control = 0;
  int radius_pill = 0;
  int radius_popup = 0;
  int panel_padding = 0;
  int row_height = 0;
  int button_size = 0;
  int icon_size = 0;
  int font_body = 0;
  int font_small = 0;

  QString font_family;
};

}  // namespace snapper
