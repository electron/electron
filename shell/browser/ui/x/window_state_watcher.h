// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_X_WINDOW_STATE_WATCHER_H_
#define SHELL_BROWSER_UI_X_WINDOW_STATE_WATCHER_H_

#include "ui/events/platform/x11/x11_event_source.h"
#include "ui/gfx/x/x11.h"

#include "shell/browser/native_window_views.h"

namespace electron {

class WindowStateWatcher : public ui::XEventObserver {
 public:
  explicit WindowStateWatcher(NativeWindowViews* window);
  ~WindowStateWatcher() override;

 protected:
  // ui::XEventObserver:
  void WillProcessXEvent(XEvent* xev) override;
  void DidProcessXEvent(XEvent* xev) override;

 private:
  bool IsWindowStateEvent(XEvent* xev) const;

  NativeWindowViews* window_;
  gfx::AcceleratedWidget widget_;
  const x11::Atom window_state_atom_;

  bool was_minimized_ = false;
  bool was_maximized_ = false;

  DISALLOW_COPY_AND_ASSIGN(WindowStateWatcher);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_X_WINDOW_STATE_WATCHER_H_
