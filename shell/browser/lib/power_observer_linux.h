// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_LIB_POWER_OBSERVER_LINUX_H_
#define ELECTRON_SHELL_BROWSER_LIB_POWER_OBSERVER_LINUX_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/power_monitor/power_observer.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"

namespace electron {

class PowerObserverLinux {
 public:
  explicit PowerObserverLinux(base::PowerSuspendObserver* suspend_observer);
  ~PowerObserverLinux();

  // disable copy
  PowerObserverLinux(const PowerObserverLinux&) = delete;
  PowerObserverLinux& operator=(const PowerObserverLinux&) = delete;

  void SetShutdownHandler(base::RepeatingCallback<bool()> should_shutdown);

 private:
  void BlockSleep();
  void UnblockSleep();
  void BlockShutdown();
  void UnblockShutdown();

  void OnLoginServiceAvailable(bool available);
  void OnInhibitResponse(base::ScopedFD* scoped_fd, dbus::Response* response);
  void OnPrepareForSleep(dbus::Signal* signal);
  void OnPrepareForShutdown(dbus::Signal* signal);
  void OnSignalConnected(const std::string& interface,
                         const std::string& signal,
                         bool success);

  base::RepeatingCallback<bool()> should_shutdown_;
  raw_ptr<base::PowerSuspendObserver> suspend_observer_ = nullptr;

  scoped_refptr<dbus::ObjectProxy> logind_;
  std::string lock_owner_name_;
  base::ScopedFD sleep_lock_;
  base::ScopedFD shutdown_lock_;
  base::WeakPtrFactory<PowerObserverLinux> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_LIB_POWER_OBSERVER_LINUX_H_
