// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "ui/events/platform/platform_event_observer.h"

#include "atom/browser/native_window_views.h"

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
  NativeWindowViews* window_;
  gfx::AcceleratedWidget widget_;

  DISALLOW_COPY_AND_ASSIGN(WindowStateWatcher);
};

}  // namespace atom
