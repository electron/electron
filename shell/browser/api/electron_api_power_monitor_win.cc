// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_power_monitor.h"

#include <windows.h>

#include <wtsapi32.h>

#include "base/functional/bind.h"
#include "gin/persistent.h"
#include "shell/browser/javascript_environment.h"
#include "ui/base/win/session_change_observer.h"
#include "v8/include/v8-cppgc.h"

namespace electron::api {

void PowerMonitor::InitPlatformSpecificMonitors() {
  // Suspend and resume come from base::PowerMonitor; session lock and unlock
  // come from WTS session change notifications delivered on the UI thread.
  auto& allocation_handle =
      JavascriptEnvironment::GetIsolate()->GetCppHeap()->GetAllocationHandle();
  session_change_observer_ =
      std::make_unique<ui::SessionChangeObserver>(base::BindRepeating(
          &PowerMonitor::OnSessionChange,
          gin::WrapPersistent(weak_factory_.GetWeakCell(allocation_handle))));
}

void PowerMonitor::DestroyPlatformSpecificMonitors() {
  session_change_observer_.reset();
}

void PowerMonitor::OnSessionChange(WPARAM wparam,
                                   const bool* is_current_session) {
  if (is_current_session && !*is_current_session)
    return;

  const char* event_name = nullptr;
  if (wparam == WTS_SESSION_LOCK)
    event_name = "lock-screen";
  else if (wparam == WTS_SESSION_UNLOCK)
    event_name = "unlock-screen";
  if (event_name)
    Emit(event_name);
}

}  // namespace electron::api
