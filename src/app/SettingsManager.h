#pragma once

#include <QByteArray>

#include <memory>

#include "edit/tools/PenTool.h"
#include "edit/tools/ShapeTool.h"

class QSettings;

namespace snapper {

// Owns the choices the app remembers between runs. Each one has a getter
// and a setter here, so nothing else reads or writes QSettings keys.
class SettingsManager {
 public:
  // Bump when panels are added, removed or renamed, or the gap between them
  // changes, so an old saved layout is dropped instead of restored wrong.
  static constexpr int kLayoutVersion = 2;

  // Uses the app's normal settings file.
  SettingsManager();
  // Uses the given store, for tests.
  explicit SettingsManager(std::unique_ptr<QSettings> store);
  ~SettingsManager();
  SettingsManager(const SettingsManager&) = delete;
  SettingsManager& operator=(const SettingsManager&) = delete;

  // Empty when nothing was saved or the saved layout is from an older
  // version.
  QByteArray PanelLayout() const;
  void SetPanelLayout(const QByteArray& layout);

  // The pen's last settings, or its defaults on first run.
  PenSettings Pen() const;
  void SetPen(const PenSettings& pen);

  // A shape tool's last settings, by tool id ("rectangle", "ellipse").
  ShapeToolSettings ShapeTool(const QString& tool) const;
  void SetShapeTool(const QString& tool, const ShapeToolSettings& settings);

 private:
  std::unique_ptr<QSettings> store_;
};

}  // namespace snapper
