#pragma once

#include <QWidget>

#include "app/Theme.h"

class QCheckBox;
class QComboBox;
class QPushButton;

namespace snapper {

class CanvasView;
class Document;
class KeyActions;
class NumberField;
class Playhead;
class TimelineView;

// The Timeline panel: play, the current frame, the curve of the selected
// keys and onion skin along the top, and the frames and keys below.
class TimelinePanel : public QWidget {
  Q_OBJECT

 public:
  TimelinePanel(const Document& document, Playhead& playhead, KeyActions& keys,
                CanvasView& canvas, const Theme& theme,
                QWidget* parent = nullptr);

 private:
  void RefreshFrame();
  void RefreshCurve();
  void RefreshOnion();

  Playhead& playhead_;
  KeyActions& keys_;
  CanvasView& canvas_;
  QPushButton* play_;
  NumberField* frame_;
  QComboBox* curve_;
  QCheckBox* onion_;
  NumberField* onion_before_;
  NumberField* onion_after_;
  TimelineView* view_;
};

}  // namespace snapper
