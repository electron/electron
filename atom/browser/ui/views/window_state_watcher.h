// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "ui/events/platform/platform_event_observer.h"

#include "atom/browser/native_window_views.h"

#if defined(USE_X11)
#include "ui/gfx/x/x11_atom_cache.h"
#endif

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

#if defined(USE_X11)
  ui::X11AtomCache atom_cache_;
#endif

  bool was_minimized_;
  bool was_maximized_;

  DISALLOW_COPY_AND_ASSIGN(WindowStateWatcher);
};

}  // namespace atom
