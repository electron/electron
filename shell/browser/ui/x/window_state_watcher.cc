// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/x/window_state_watcher.h"

#include <vector>

#include "ui/base/x/x11_util.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/gfx/x/xproto_util.h"

namespace electron {

WindowStateWatcher::WindowStateWatcher(NativeWindowViews* window)
    : window_(window),
      widget_(window->GetAcceleratedWidget()),
      net_wm_state_atom_(x11::GetAtom("_NET_WM_STATE")),
      net_wm_state_hidden_atom_(x11::GetAtom("_NET_WM_STATE_HIDDEN")),
      net_wm_state_maximized_vert_atom_(
          x11::GetAtom("_NET_WM_STATE_MAXIMIZED_VERT")),
      net_wm_state_maximized_horz_atom_(
          x11::GetAtom("_NET_WM_STATE_MAXIMIZED_HORZ")),
      net_wm_state_fullscreen_atom_(x11::GetAtom("_NET_WM_STATE_FULLSCREEN")) {
  ui::X11EventSource::GetInstance()->connection()->AddEventObserver(this);
}

WindowStateWatcher::~WindowStateWatcher() {
  ui::X11EventSource::GetInstance()->connection()->RemoveEventObserver(this);
}

void WindowStateWatcher::OnEvent(const x11::Event& x11_event) {
  if (IsWindowStateEvent(x11_event)) {
    const bool was_minimized_ = window_->IsMinimized();
    const bool was_maximized_ = window_->IsMaximized();

    std::vector<x11::Atom> wm_states;
    if (GetArrayProperty(
            static_cast<x11::Window>(window_->GetAcceleratedWidget()),
            net_wm_state_atom_, &wm_states)) {
      const auto props =
          base::flat_set<x11::Atom>(std::begin(wm_states), std::end(wm_states));
      const bool is_minimized = props.contains(net_wm_state_hidden_atom_);
      const bool is_maximized =
          props.contains(net_wm_state_maximized_vert_atom_) &&
          props.contains(net_wm_state_maximized_horz_atom_);
      const bool is_fullscreen = props.contains(net_wm_state_fullscreen_atom_);

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
  return (property && property->atom == net_wm_state_atom_ &&
          static_cast<uint32_t>(property->window) == widget_);
}

}  // namespace electron
