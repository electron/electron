// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_CRASH_REPORTER_H_
#define SHELL_BROWSER_API_ELECTRON_API_CRASH_REPORTER_H_

#include <map>
#include <string>

namespace electron {

namespace api {

namespace crash_reporter {

bool IsCrashReporterEnabled();

#if defined(OS_LINUX)
const std::map<std::string, std::string>& GetGlobalCrashKeys();
#endif

}  // namespace crash_reporter

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_NET_H_
