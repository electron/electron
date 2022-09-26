// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CRASH_REPORTER_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CRASH_REPORTER_H_

#include <map>
#include <string>
#include "base/files/file_path.h"

namespace electron::api::crash_reporter {

bool IsCrashReporterEnabled();

#if BUILDFLAG(IS_LINUX)
const std::map<std::string, std::string>& GetGlobalCrashKeys();
std::string GetClientId();
#endif

// JS bindings API; exposed publicly because it's also called from node_main.cc
void Start(const std::string& submit_url,
           bool upload_to_server,
           bool ignore_system_crash_handler,
           bool rate_limit,
           bool compress,
           const std::map<std::string, std::string>& global_extra,
           const std::map<std::string, std::string>& extra,
           bool is_node_process);

}  // namespace electron::api::crash_reporter

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CRASH_REPORTER_H_
