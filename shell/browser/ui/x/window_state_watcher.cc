// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/x/window_state_watcher.h"
#include "ui/gfx/x/x11_atom_cache.h"

namespace electron {

WindowStateWatcher::WindowStateWatcher(NativeWindowViews* window)
    : window_(window),
      widget_(window->GetAcceleratedWidget()),
      window_state_atom_(gfx::GetAtom("_NET_WM_STATE")) {
  ui::X11EventSource::GetInstance()->AddXEventObserver(this);
}

WindowStateWatcher::~WindowStateWatcher() {
  ui::X11EventSource::GetInstance()->RemoveXEventObserver(this);
}

void WindowStateWatcher::WillProcessXEvent(x11::Event* x11_event) {
  if (IsWindowStateEvent(x11_event)) {
    was_minimized_ = window_->IsMinimized();
    was_maximized_ = window_->IsMaximized();
  }
}

void WindowStateWatcher::DidProcessXEvent(x11::Event* x11_event) {
  if (IsWindowStateEvent(x11_event)) {
    bool is_minimized = window_->IsMinimized();
    bool is_maximized = window_->IsMaximized();
    bool is_fullscreen = window_->IsFullscreen();
    if (is_minimized != was_minimized_) {
      if (is_minimized)
        window_->NotifyWindowMinimize();
      else
        window_->NotifyWindowRestore();
    } else if (is_maximized != was_maximized_) {
      if (is_maximized)
        window_->NotifyWindowMaximize();
      else
        window_->NotifyWindowUnmaximize();
    } else {
      // If this is neither a "maximize" or "minimize" event, then we think it
      // is a "fullscreen" event.
      // The "IsFullscreen()" becomes true immediately before "WillProcessEvent"
      // is called, so we can not handle this like "maximize" and "minimize" by
      // watching whether they have changed.
      if (is_fullscreen)
        window_->NotifyWindowEnterFullScreen();
      else
        window_->NotifyWindowLeaveFullScreen();
    }
  }
}

bool WindowStateWatcher::IsWindowStateEvent(x11::Event* x11_event) const {
  XEvent* xev = &x11_event->xlib_event();
  return (static_cast<x11::Atom>(xev->xproperty.atom) == window_state_atom_ &&
          xev->type == PropertyNotify &&
          xev->xproperty.window == static_cast<uint32_t>(widget_));
}

}  // namespace electron
