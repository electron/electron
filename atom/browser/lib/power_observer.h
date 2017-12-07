// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_LIB_POWER_OBSERVER_H_
#define ATOM_BROWSER_LIB_POWER_OBSERVER_H_

#include "base/macros.h"

#if defined(OS_LINUX)
#include "atom/browser/lib/power_observer_linux.h"
#else
#include "base/power_monitor/power_observer.h"
#endif  // defined(OS_LINUX)

namespace atom {

#if defined(OS_LINUX)
typedef PowerObserverLinux PowerObserver;
#else
class PowerObserver : public base::PowerObserver {
 public:
  PowerObserver() {}
  void BlockShutdown() {}
  void UnblockShutdown() {}
  // Notification that the system is rebooting or shutting down. If the
  // implementation returns true, the PowerObserver instance should try to delay
  // OS shutdown so the application can perform cleanup before exiting.
  virtual bool OnShutdown() { return false; }

 private:
  DISALLOW_COPY_AND_ASSIGN(PowerObserver);
};
#endif  // defined(OS_LINUX)

}  // namespace atom

#endif  // ATOM_BROWSER_LIB_POWER_OBSERVER_H_
