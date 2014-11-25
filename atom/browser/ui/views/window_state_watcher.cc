// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/window_state_watcher.h"

#include "ui/events/platform/platform_event_source.h"

namespace atom {

WindowStateWatcher::WindowStateWatcher(NativeWindowViews* window)
    : window_(window),
      widget_(window->GetAcceleratedWidget()) {
  ui::PlatformEventSource::GetInstance()->AddPlatformEventObserver(this);
}

WindowStateWatcher::~WindowStateWatcher() {
  ui::PlatformEventSource::GetInstance()->RemovePlatformEventObserver(this);
}

void WindowStateWatcher::WillProcessEvent(const ui::PlatformEvent& event) {
  LOG(ERROR) << "WillProcessEvent";
}

void WindowStateWatcher::DidProcessEvent(const ui::PlatformEvent& event) {
}

}  // namespace atom
