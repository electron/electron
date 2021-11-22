// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_CRASH_KEYS_H_
#define ELECTRON_SHELL_COMMON_CRASH_KEYS_H_

#include <map>
#include <string>

namespace base {
class CommandLine;
}

namespace electron {

namespace crash_keys {

void SetCrashKey(const std::string& key, const std::string& value);
void ClearCrashKey(const std::string& key);
void GetCrashKeys(std::map<std::string, std::string>* keys);

void SetCrashKeysFromCommandLine(const base::CommandLine& command_line);
void SetPlatformCrashKey();

}  // namespace crash_keys

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_CRASH_KEYS_H_
