// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "ui/events/platform/platform_event_observer.h"

#include "atom/browser/native_window_views.h"
#include "ui/gfx/x/x11_atom_cache.h"

namespace atom {

class WindowStateWatcher : public ui::PlatformEventObserver {
 public:
  explicit WindowStateWatcher(NativeWindowViews* window);
  virtual ~WindowStateWatcher();

 protected:
  // ui::PlatformEventObserver:
  void WillProcessEvent(const ui::PlatformEvent& event) override;
  void DidProcessEvent(const ui::PlatformEvent& event) override;

 private:
  bool IsWindowStateEvent(const ui::PlatformEvent& event);

  NativeWindowViews* window_;
  gfx::AcceleratedWidget widget_;

  ui::X11AtomCache atom_cache_;

  bool was_minimized_;
  bool was_maximized_;

  DISALLOW_COPY_AND_ASSIGN(WindowStateWatcher);
};

}  // namespace atom
