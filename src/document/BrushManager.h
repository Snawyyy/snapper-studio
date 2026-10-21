#pragma once

#include <QObject>
#include <QString>

#include <optional>
#include <vector>

#include "document/Ids.h"

namespace snapper {

// A brush stamps its picture along a line. The picture's alpha is how much
// paint each spot lays down, and the line's color tints it.
struct Brush {
  static constexpr float kMinSpacing = 0.02F;
  static constexpr float kMaxSpacing = 4.0F;
  static constexpr float kMaxAngleJitter = 180.0F;
  static constexpr int kMaxBoilEvery = 240;

  Id id = kNoId;
  QString name{};
  // The picture's name in the image store.
  QString image{};
  // The gap between stamps, as a share of the line width there.
  float spacing = 0.15F;
  // Turns each stamp to follow the line.
  bool align = true;
  // Random turn of each stamp, in degrees either way.
  float angle_jitter = 0.0F;
  // Random change of each gap, as a share of it.
  float spacing_jitter = 0.0F;
  // Random change of each stamp's size, as a share of it.
  float size_jitter = 0.0F;
  // Frames between new random stamps, for lines that boil. 0 keeps them.
  int boil_every = 0;

  bool operator==(const Brush&) const = default;

  // The same brush with every setting inside its range.
  Brush Clamped() const;
};

struct RemovedBrush {
  Brush brush{};
  int index = 0;
};

// Owns the brushes of a project and their settings. Their pictures live in
// the image store.
class BrushManager : public QObject {
  Q_OBJECT

 public:
  static constexpr int kMaxBrushes = 256;

  explicit BrushManager(QObject* parent = nullptr);

  // Inserts at index (clamped), with its settings clamped. Fails on a
  // missing or repeated id, or a full list.
  bool Insert(const Brush& brush, int index);
  std::optional<RemovedBrush> Take(Id id);
  // Replaces a brush's name and settings. False when no brush has its id.
  bool Set(const Brush& brush);
  void Clear();

  const Brush* Find(Id id) const;
  // -1 when no brush has this id.
  int IndexOf(Id id) const;
  int Count() const { return static_cast<int>(brushes_.size()); }
  const std::vector<Brush>& All() const { return brushes_; }

 Q_SIGNALS:
  // A brush was added or removed.
  void ListChanged();
  // A brush's name or settings changed.
  void BrushChanged(Id id);

 private:
  std::vector<Brush> brushes_;
};

}  // namespace snapper
