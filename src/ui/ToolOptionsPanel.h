#pragma once

#include <QHash>
#include <QStackedWidget>

class QLabel;

namespace snapper {

// Shows the settings of whichever tool is active. Tools without settings
// say so.
class ToolOptionsPanel : public QStackedWidget {
  Q_OBJECT

 public:
  explicit ToolOptionsPanel(QWidget* parent = nullptr);

  // Takes ownership of the page.
  void AddPage(const QString& tool_id, QWidget* page);
  void ShowTool(const QString& tool_id);

 private:
  QHash<QString, QWidget*> pages_;
  QLabel* empty_;
};

}  // namespace snapper
