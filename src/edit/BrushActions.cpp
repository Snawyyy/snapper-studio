#include "edit/BrushActions.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>

#include <algorithm>

#include "document/Document.h"
#include "edit/LookManager.h"
#include "edit/StyleCommands.h"
#include "edit/UndoManager.h"

namespace snapper {

BrushActions::BrushActions(Document& document, LookManager& looks,
                           UndoManager& undo,
                           std::function<void(const QString&)> hint)
    : document_(document), looks_(looks), undo_(undo), hint_(std::move(hint)) {}

Id BrushActions::ImportBrush(const QString& path, QString* why_not) {
  if (document_.Brushes().Count() >= BrushManager::kMaxBrushes) {
    *why_not = QStringLiteral("A project holds at most %1 brushes")
                   .arg(BrushManager::kMaxBrushes);
    return kNoId;
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    *why_not = QStringLiteral("Can't open %1").arg(path);
    return kNoId;
  }
  if (file.size() > ImageStore::kMaxBytes) {
    *why_not = QStringLiteral("The picture is larger than 256 MB");
    return kNoId;
  }
  const QByteArray bytes = file.readAll();
  QBuffer buffer;
  buffer.setData(bytes);
  const QSize size = QImageReader(&buffer).size();
  const bool is_too_big =
      size.width() > kMaxPictureSide || size.height() > kMaxPictureSide;
  if (is_too_big) {
    *why_not = QStringLiteral("A brush picture is at most %1 pixels on a side")
                   .arg(kMaxPictureSide);
    return kNoId;
  }
  const std::optional<QString> name = document_.Images().Add(bytes, why_not);
  if (!name.has_value()) {
    return kNoId;
  }
  const Brush brush{.id = document_.NewId(),
                    .name = QFileInfo(path).completeBaseName(),
                    .image = *name};
  undo_.Push(std::make_unique<AddBrushCommand>(document_, brush,
                                               document_.Brushes().Count()));
  hint_(QStringLiteral("Added the brush %1").arg(brush.name));
  return brush.id;
}

void BrushActions::DeleteBrush(Id brush) {
  if (document_.Brushes().Find(brush) != nullptr) {
    undo_.Push(std::make_unique<RemoveBrushCommand>(document_, brush));
  }
}

void BrushActions::RenameBrush(Id brush, const QString& name) {
  const QString trimmed = name.trimmed();
  if (trimmed.isEmpty()) {
    return;
  }
  EditBrush(brush, [&](Brush& edited) { edited.name = trimmed; });
}

void BrushActions::EditBrush(Id brush, const std::function<void(Brush&)>& edit,
                             int drag_tag) {
  const Brush* found = document_.Brushes().Find(brush);
  if (found == nullptr) {
    return;
  }
  Brush after = *found;
  edit(after);
  after.id = brush;
  after.image = found->image;
  after = after.Clamped();
  if (after == *found) {
    return;
  }
  undo_.Push(
      std::make_unique<SetBrushCommand>(document_, *found, after, drag_tag));
}

void BrushActions::UseBrush(Id brush) {
  const QString style = looks_.StyleSetting(LookPart::kBrush);
  if (!style.isEmpty()) {
    hint_(QStringLiteral("The brush comes from the style %1: edit the style "
                         "to change it")
              .arg(style));
    return;
  }
  looks_.Change(QStringLiteral("Brush"),
                [brush](ShapeLookState& state) { state.look.brush = brush; });
}

QString BrushActions::DeleteEffect(Id brush) const {
  const int shapes =
      static_cast<int>(document_.Shapes().ShapesWithBrush(brush).size());
  const int styles = static_cast<int>(std::ranges::count_if(
      document_.Styles().All(),
      [brush](const Style& style) { return style.look.brush == brush; }));
  if (shapes + styles == 0) {
    return QStringLiteral("Delete the brush. Nothing uses it.");
  }
  return QStringLiteral(
             "Delete the brush. %1 shapes and %2 styles using it draw plain "
             "lines.")
      .arg(shapes)
      .arg(styles);
}

}  // namespace snapper
