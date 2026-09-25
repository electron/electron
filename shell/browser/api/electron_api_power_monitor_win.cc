// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_power_monitor.h"

#include <windows.h>

#include <wtsapi32.h>

#include "base/functional/bind.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/win/session_change_observer.h"

namespace electron::api {

void PowerMonitor::InitPlatformSpecificMonitors() {
  // Suspend and resume come from base::PowerMonitor; session lock and unlock
  // come from WTS session change notifications on the UI thread's window.
  // The JS module keeps this object alive, so Unretained is safe.
  session_change_observer_ =
      std::make_unique<ui::SessionChangeObserver>(base::BindRepeating(
          &PowerMonitor::OnSessionChange, base::Unretained(this)));
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
  if (!event_name)
    return;

  // Emit outside of the window procedure that delivered the notification.
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(
                     [](PowerMonitor* power_monitor, const char* event_name) {
                       power_monitor->Emit(event_name);
                     },
                     base::Unretained(this), event_name));
}

}  // namespace electron::api
