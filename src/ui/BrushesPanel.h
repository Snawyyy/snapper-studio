#pragma once

#include <QWidget>

#include <functional>
#include <memory>
#include <vector>

#include "app/Theme.h"
#include "document/BrushManager.h"
#include "ui/DragTagger.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

namespace snapper {

class BrushActions;
class Document;
class NumberField;

// The Brushes panel: every brush with a sample stroke, and the picked
// brush's settings below, always in that place. Changes show at once on
// the canvas and in the sample.
class BrushesPanel : public QWidget {
  Q_OBJECT

 public:
  BrushesPanel(Document& document, BrushActions& actions, const Theme& theme,
               QWidget* parent = nullptr);

  // Asks for a picture file and adds it as a brush.
  void ImportBrush();

 private:
  void Refill();
  void ShowBrush();
  Id CurrentBrush() const;
  // Ties a number field to one brush setting. Each field gets its own drag
  // tag, so scrubbing it is one undo step.
  void Bind(NumberField* field, const QString& what,
            const std::function<void(Brush&, double)>& set);

  Document& document_;
  BrushActions& actions_;
  const Theme& theme_;
  QListWidget* list_;
  QPushButton* import_;
  QPushButton* use_;
  QPushButton* delete_;
  QWidget* editor_;
  QLineEdit* name_;
  QLabel* sample_;
  NumberField* spacing_;
  QCheckBox* align_;
  NumberField* angle_jitter_;
  NumberField* spacing_jitter_;
  NumberField* size_jitter_;
  NumberField* boil_every_;
  std::vector<std::unique_ptr<DragTagger>> drags_;
};

}  // namespace snapper
