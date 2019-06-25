// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_LIB_POWER_OBSERVER_H_
#define SHELL_BROWSER_LIB_POWER_OBSERVER_H_

#include "base/macros.h"

#if defined(OS_LINUX)
#include "shell/browser/lib/power_observer_linux.h"
#else
#include "base/power_monitor/power_observer.h"
#endif  // defined(OS_LINUX)

namespace electron {

#if defined(OS_LINUX)
typedef PowerObserverLinux PowerObserver;
#else
typedef base::PowerObserver PowerObserver;
#endif  // defined(OS_LINUX)

}  // namespace electron

#endif  // SHELL_BROWSER_LIB_POWER_OBSERVER_H_
