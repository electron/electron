// Copyright (c) 2023 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEB_CONTENTS_ZOOM_OBSERVER_H_
#define ELECTRON_SHELL_BROWSER_WEB_CONTENTS_ZOOM_OBSERVER_H_

#include "shell/browser/web_contents_zoom_controller.h"

namespace electron {

// Interface for objects that wish to be notified of changes in
// WebContentsZoomController.
class WebContentsZoomObserver : public base::CheckedObserver {
 public:
  // Fired when the ZoomController is destructed. Observers should deregister
  // themselves from the ZoomObserver in this event handler. Note that
  // ZoomController::FromWebContents() returns nullptr at this point already.
  virtual void OnZoomControllerDestroyed(
      WebContentsZoomController* zoom_controller) = 0;

  // Notification that the zoom percentage has changed.
  virtual void OnZoomChanged(
      const WebContentsZoomController::ZoomChangedEventData& data) {}
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WEB_CONTENTS_ZOOM_OBSERVER_H_
