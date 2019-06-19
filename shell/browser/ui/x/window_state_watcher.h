// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_X_WINDOW_STATE_WATCHER_H_
#define SHELL_BROWSER_UI_X_WINDOW_STATE_WATCHER_H_

#include "ui/events/platform/platform_event_observer.h"

#include "shell/browser/native_window_views.h"

namespace electron {

class WindowStateWatcher : public ui::PlatformEventObserver {
 public:
  explicit WindowStateWatcher(NativeWindowViews* window);
  ~WindowStateWatcher() override;

 protected:
  // ui::PlatformEventObserver:
  void WillProcessEvent(const ui::PlatformEvent& event) override;
  void DidProcessEvent(const ui::PlatformEvent& event) override;

 private:
  bool IsWindowStateEvent(const ui::PlatformEvent& event);

  NativeWindowViews* window_;
  gfx::AcceleratedWidget widget_;

  bool was_minimized_ = false;
  bool was_maximized_ = false;

  DISALLOW_COPY_AND_ASSIGN(WindowStateWatcher);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_X_WINDOW_STATE_WATCHER_H_
