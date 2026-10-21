#pragma once

#include <QFrame>
#include <QPoint>

class QButtonGroup;
class QLineEdit;

namespace snapper {

class StudyManager;

// A small box for a study note: what you are trying to do, and how
// frustrated you are from 1 to 5. It opens pinned to where the pointer
// rested, inside the window. Enter saves, Escape closes.
class StudyNoteBox : public QFrame {
  Q_OBJECT

 public:
  StudyNoteBox(StudyManager& study, QWidget* parent);

  void Open();

 protected:
  void keyPressEvent(QKeyEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void SaveNote();

  StudyManager& study_;
  QLineEdit* text_;
  QButtonGroup* frustration_;
  QPoint spot_;
};

}  // namespace snapper
