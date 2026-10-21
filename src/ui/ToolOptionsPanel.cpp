#include "ui/ToolOptionsPanel.h"

#include <QLabel>

namespace snapper {

ToolOptionsPanel::ToolOptionsPanel(QWidget* parent)
    : QStackedWidget(parent),
      empty_(new QLabel(QStringLiteral("This tool has no settings"), this)) {
  setObjectName(QStringLiteral("tool-options"));
  empty_->setObjectName(QStringLiteral("secondary"));
  empty_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  addWidget(empty_);
}

void ToolOptionsPanel::AddPage(const QString& tool_id, QWidget* page) {
  addWidget(page);
  pages_.insert(tool_id, page);
}

void ToolOptionsPanel::ShowTool(const QString& tool_id) {
  setCurrentWidget(pages_.value(tool_id, empty_));
}

}  // namespace snapper
