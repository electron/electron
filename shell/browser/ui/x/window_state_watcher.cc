// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/x/window_state_watcher.h"

#include <vector>

#include "ui/base/x/x11_util.h"
#include "ui/gfx/x/x11_atom_cache.h"

namespace electron {

WindowStateWatcher::WindowStateWatcher(NativeWindowViews* window)
    : window_(window),
      widget_(window->GetAcceleratedWidget()),
      window_state_atom_(x11::GetAtom("_NET_WM_STATE")) {
  ui::X11EventSource::GetInstance()->connection()->AddEventObserver(this);
}

WindowStateWatcher::~WindowStateWatcher() {
  ui::X11EventSource::GetInstance()->connection()->RemoveEventObserver(this);
}

void WindowStateWatcher::OnEvent(const x11::Event& x11_event) {
  if (IsWindowStateEvent(x11_event)) {
    bool was_minimized_ = window_->IsMinimized();
    bool was_maximized_ = window_->IsMaximized();

    std::vector<x11::Atom> wm_states;

    if (ui::GetAtomArrayProperty(
            static_cast<x11::Window>(window_->GetAcceleratedWidget()),
            "_NET_WM_STATE", &wm_states)) {
      auto props =
          base::flat_set<x11::Atom>(std::begin(wm_states), std::end(wm_states));
      bool is_minimized =
          props.find(x11::GetAtom("_NET_WM_STATE_HIDDEN")) != props.end();
      bool is_maximized =
          props.find(x11::GetAtom("_NET_WM_STATE_MAXIMIZED_VERT")) !=
              props.end() &&
          props.find(x11::GetAtom("_NET_WM_STATE_MAXIMIZED_HORZ")) !=
              props.end();
      bool is_fullscreen =
          props.find(x11::GetAtom("_NET_WM_STATE_FULLSCREEN")) != props.end();

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
        // The "IsFullscreen()" becomes true immediately before "OnEvent"
        // is called, so we can not handle this like "maximize" and "minimize"
        // by watching whether they have changed.
        if (is_fullscreen)
          window_->NotifyWindowEnterFullScreen();
        else
          window_->NotifyWindowLeaveFullScreen();
      }
    }
  }
}

bool WindowStateWatcher::IsWindowStateEvent(const x11::Event& x11_event) const {
  auto* property = x11_event.As<x11::PropertyNotifyEvent>();
  return (property && property->atom == window_state_atom_ &&
          static_cast<uint32_t>(property->window) == widget_);
}

}  // namespace electron
