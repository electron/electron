// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/x/window_state_watcher.h"

#include <X11/Xlib.h>

#include "ui/events/platform/platform_event_source.h"

namespace atom {

namespace {

const char* kAtomsToCache[] = {
  "_NET_WM_STATE",
  NULL,
};

}  // namespace

WindowStateWatcher::WindowStateWatcher(NativeWindowViews* window)
    : window_(window),
      widget_(window->GetAcceleratedWidget()),
      atom_cache_(gfx::GetXDisplay(), kAtomsToCache),
      was_minimized_(false),
      was_maximized_(false) {
  ui::PlatformEventSource::GetInstance()->AddPlatformEventObserver(this);
}

WindowStateWatcher::~WindowStateWatcher() {
  ui::PlatformEventSource::GetInstance()->RemovePlatformEventObserver(this);
}

void WindowStateWatcher::WillProcessEvent(const ui::PlatformEvent& event) {
  if (IsWindowStateEvent(event)) {
    was_minimized_ = window_->IsMinimized();
    was_maximized_ = window_->IsMaximized();
  }
}

void WindowStateWatcher::DidProcessEvent(const ui::PlatformEvent& event) {
  if (IsWindowStateEvent(event)) {
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

bool WindowStateWatcher::IsWindowStateEvent(const ui::PlatformEvent& event) {
  ::Atom changed_atom = event->xproperty.atom;
  return (changed_atom == atom_cache_.GetAtom("_NET_WM_STATE") &&
          event->type == PropertyNotify &&
          event->xproperty.window == widget_);
}

}  // namespace atom
