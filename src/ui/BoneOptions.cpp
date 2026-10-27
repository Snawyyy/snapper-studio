#include "ui/BoneOptions.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include "document/Document.h"
#include "edit/BoneActions.h"
#include "edit/SelectionManager.h"

namespace snapper {

BoneOptions::BoneOptions(Document& document, SelectionManager& selection,
                         BoneActions& actions, QWidget* parent)
    : QWidget(parent),
      document_(document),
      actions_(actions),
      name_(new QLineEdit(this)),
      pinned_(new QCheckBox(QStringLiteral("Pinned"), this)),
      note_(new QLabel(this)) {
  name_->setObjectName(QStringLiteral("bone-name"));
  pinned_->setObjectName(QStringLiteral("bone-pinned"));
  pinned_->setToolTip(QStringLiteral(
      "A pinned bone stays put: pulling a bone below it bends only the "
      "bones between"));
  note_->setObjectName(QStringLiteral("secondary"));
  note_->setWordWrap(true);

  auto* form = new QFormLayout();
  form->addRow(QStringLiteral("Bone"), name_);
  auto* layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addWidget(pinned_);
  layout->addWidget(note_);
  layout->addStretch();

  connect(name_, &QLineEdit::editingFinished, this, [this] {
    actions_.RenameBone(actions_.SingleSelected(), name_->text());
  });
  connect(pinned_, &QCheckBox::clicked, this, [this](bool checked) {
    actions_.SetPinned(actions_.SingleSelected(), checked);
  });
  connect(&selection, &SelectionManager::BonesChanged, this,
          &BoneOptions::Refresh);
  connect(&document_.Skeletons(), &SkeletonManager::BonesChanged, this,
          &BoneOptions::Refresh);
  Refresh();
}

void BoneOptions::Refresh() {
  const Bone* bone = document_.Skeletons().Find(actions_.SingleSelected());
  const bool has_bone = bone != nullptr;
  name_->setEnabled(has_bone);
  pinned_->setEnabled(has_bone);
  if (!name_->hasFocus()) {
    name_->setText(has_bone ? bone->name : QString());
  }
  pinned_->setChecked(has_bone && bone->pinned);
  note_->setText(has_bone
                     ? QString()
                     : QStringLiteral("Select one bone to name or pin it"));
  note_->setVisible(!has_bone);
}

}  // namespace snapper
