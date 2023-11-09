// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_TRAY_ICON_OBSERVER_H_
#define ELECTRON_SHELL_BROWSER_UI_TRAY_ICON_OBSERVER_H_

#include <string>
#include <vector>

#include "base/observer_list_types.h"

namespace gfx {
class Rect;
class Point;
}  // namespace gfx

namespace electron {

class TrayIconObserver : public base::CheckedObserver {
 public:
  virtual void OnClicked(const gfx::Rect& bounds,
                         const gfx::Point& location,
                         int modifiers) {}
  virtual void OnDoubleClicked(const gfx::Rect& bounds, int modifiers) {}
  virtual void OnMiddleClicked(const gfx::Rect& bounds, int modifiers) {}
  virtual void OnBalloonShow() {}
  virtual void OnBalloonClicked() {}
  virtual void OnBalloonClosed() {}
  virtual void OnRightClicked(const gfx::Rect& bounds, int modifiers) {}
  virtual void OnDrop() {}
  virtual void OnDropFiles(const std::vector<std::string>& files) {}
  virtual void OnDropText(const std::string& text) {}
  virtual void OnDragEntered() {}
  virtual void OnDragExited() {}
  virtual void OnDragEnded() {}
  virtual void OnMouseUp(const gfx::Point& location, int modifiers) {}
  virtual void OnMouseDown(const gfx::Point& location, int modifiers) {}
  virtual void OnMouseEntered(const gfx::Point& location, int modifiers) {}
  virtual void OnMouseExited(const gfx::Point& location, int modifiers) {}
  virtual void OnMouseMoved(const gfx::Point& location, int modifiers) {}

 protected:
  ~TrayIconObserver() override {}
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_TRAY_ICON_OBSERVER_H_
