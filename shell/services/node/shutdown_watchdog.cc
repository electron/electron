// Copyright (c) 2024 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/services/node/shutdown_watchdog.h"

#include "base/logging.h"
#include "base/process/process.h"

namespace electron {

ShutdownWatchdog::ShutdownWatchdog(const base::TimeDelta& duration,
                                   int exit_code)
    : exit_code_(exit_code),
      watchdog_(duration, "Shutdown watchdog", true, this) {}

void ShutdownWatchdog::Arm() {
  watchdog_.Arm();
}

void ShutdownWatchdog::Alarm() {
  LOG(ERROR) << "Shutdown watchdog triggered, exiting with code " << exit_code_;
  base::Process::TerminateCurrentProcessImmediately(exit_code_);
}

}  // namespace electron
