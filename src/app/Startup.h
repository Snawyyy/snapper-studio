#pragma once

class QApplication;

namespace snapper {

// Names the app, applies the theme and starts the docking system. The app
// and the UI driver both call this so they start the same way.
void SetUpApplication(QApplication& app);

}  // namespace snapper
