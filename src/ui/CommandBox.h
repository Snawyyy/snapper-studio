#pragma once

#include <QFrame>

class QLineEdit;
class QListWidget;

namespace snapper {

class CommandRegistry;

// The Ctrl+K search box: type part of a command's name, pick it with the
// arrows or the pointer, Enter runs it, Escape closes. It floats over the
// top of its parent window and never leaves it.
class CommandBox : public QFrame {
  Q_OBJECT

 public:
  CommandBox(CommandRegistry* registry, QWidget* parent);

  void Open();
  void Close();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void Refill();
  void RunCurrent();
  void PlaceInParent();

  CommandRegistry* registry_;
  QLineEdit* search_;
  QListWidget* results_;
};

}  // namespace snapper
