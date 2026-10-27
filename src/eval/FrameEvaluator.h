#pragma once

#include <QObject>

#include <unordered_map>

#include "eval/Pose.h"

namespace snapper {

class Document;
class Playhead;

// Turns the document and a frame into a pose: every channel read on that
// frame. Poses are kept until the document changes.
class FrameEvaluator : public QObject {
  Q_OBJECT

 public:
  // Room for the current pose and every onion skin frame.
  static constexpr int kMaxKeptPoses = 32;

  FrameEvaluator(const Document& document, const Playhead& playhead,
                 QObject* parent = nullptr);

  static Pose Evaluate(const Document& document, int frame);
  static LayerTransform TransformAt(const Document& document,
                                    const Layer& layer, int frame);

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
};

}  // namespace snapper
