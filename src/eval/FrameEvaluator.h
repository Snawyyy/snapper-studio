#pragma once

#include <QObject>

#include <unordered_map>

#include "eval/Pose.h"

namespace snapper {

class Document;
class Playhead;

// Turns the document and a frame into a pose: every channel read on that
// frame, bones posed parents first, and layers moved by the bones they
// follow. Poses are kept until the document changes.
class FrameEvaluator : public QObject {
  Q_OBJECT

 public:
  // Room for the current pose and every onion skin frame.
  static constexpr int kMaxKeptPoses = 32;

  FrameEvaluator(const Document& document, const Playhead& playhead,
                 QObject* parent = nullptr);

  // Bones in `trial` stand that way instead of as the document says.
  static Pose Evaluate(const Document& document, int frame,
                       const QHash<Id, BoneTransform>& trial = {});
  static LayerTransform TransformAt(const Document& document,
                                    const Layer& layer, int frame);
  static BoneTransform BoneTransformAt(const Document& document,
                                       const Bone& bone, int frame);

  // A trial pose for bones on the setup frame, shown but never saved, as
  // when bending bones there to test the rig. Empty ends it.
  void SetTrial(QHash<Id, BoneTransform> trial);
  const QHash<Id, BoneTransform>& Trial() const { return trial_; }

  // Stays valid until the document changes, while fewer than
  // kMaxKeptPoses other frames are asked for.
  const Pose& At(int frame);
  // On the playhead's frame.
  const Pose& Current();

 Q_SIGNALS:
  // The current pose is different: the document changed, or the playhead
  // moved.
  void PoseChanged();

 private:
  void DocumentChanged();

  const Document& document_;
  const Playhead& playhead_;
  // Node based, so adding a pose never moves the others.
  std::unordered_map<int, Pose> kept_;
  QHash<Id, BoneTransform> trial_;
};

}  // namespace snapper
