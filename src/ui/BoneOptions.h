#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;

namespace snapper {

class BoneActions;
class Document;
class SelectionManager;

// The selected bone's name and pin in the Tool Options panel, shared by
// the bone tools. Greyed out, saying why, unless one bone is selected.
class BoneOptions : public QWidget {
  Q_OBJECT

 public:
  BoneOptions(Document& document, SelectionManager& selection,
              BoneActions& actions, QWidget* parent = nullptr);

 private:
  void Refresh();

  Document& document_;
  BoneActions& actions_;
  QLineEdit* name_;
  QCheckBox* pinned_;
  QLabel* note_;
};

}  // namespace snapper
