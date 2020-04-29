// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/crash_keys.h"

#include <deque>
#include <vector>

#include "base/command_line.h"
#include "base/no_destructor.h"
#include "base/strings/string_split.h"
#include "components/crash/core/common/crash_key.h"

namespace electron {

namespace crash_keys {

namespace {

using ExtraCrashKeys = std::deque<crash_reporter::CrashKeyString<64>>;
ExtraCrashKeys& GetExtraCrashKeys() {
  static base::NoDestructor<ExtraCrashKeys> extra_keys;
  return *extra_keys;
}

std::deque<std::string>& GetExtraCrashKeyNames() {
  static base::NoDestructor<std::deque<std::string>> crash_keys_names;
  return *crash_keys_names;
}

}  // namespace

void SetCrashKey(const std::string& key, const std::string& value) {
  auto& crash_keys_names = GetExtraCrashKeyNames();

  auto iter = std::find(crash_keys_names.begin(), crash_keys_names.end(), key);
  if (iter == crash_keys_names.end()) {
    crash_keys_names.emplace_back(key);
    GetExtraCrashKeys().emplace_back(crash_keys_names.back().c_str());
    iter = crash_keys_names.end() - 1;
  }
  GetExtraCrashKeys()[iter - crash_keys_names.begin()].Set(value);
}

void ClearCrashKey(const std::string& key) {
  auto& crash_keys_names = GetExtraCrashKeyNames();

  auto iter = std::find(crash_keys_names.begin(), crash_keys_names.end(), key);
  if (iter != crash_keys_names.end()) {
    GetExtraCrashKeys()[iter - crash_keys_names.begin()].Clear();
  }
}

void SetCrashKeysFromCommandLine(const base::CommandLine& command_line) {
  if (command_line.HasSwitch("global-crash-keys")) {
    std::vector<std::pair<std::string, std::string>> global_crash_keys;
    base::SplitStringIntoKeyValuePairs(
        command_line.GetSwitchValueASCII("global-crash-keys"), '=', ',',
        &global_crash_keys);
    for (const auto& pair : global_crash_keys) {
      SetCrashKey(pair.first, pair.second);
    }
  }
}

}  // namespace crash_keys

}  // namespace electron
