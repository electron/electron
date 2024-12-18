// Copyright (c) 2024 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_SERVICES_NODE_SHUTDOWN_WATCHDOG_H_
#define ELECTRON_SHELL_SERVICES_NODE_SHUTDOWN_WATCHDOG_H_

#include "base/threading/watchdog.h"

namespace electron {

class ShutdownWatchdog : public base::Watchdog::Delegate {
 public:
  // Creates a watchdog that waits for |duration| (after the watchdog is
  // armed) before shutting down the process.
  explicit ShutdownWatchdog(const base::TimeDelta& duration, int exit_code);

  ShutdownWatchdog(const ShutdownWatchdog&) = delete;
  ShutdownWatchdog& operator=(const ShutdownWatchdog&) = delete;

  void Arm();
  void Alarm() override;

 private:
  int exit_code_;
  base::Watchdog watchdog_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_SERVICES_NODE_SHUTDOWN_WATCHDOG_H_
