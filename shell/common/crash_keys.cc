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

using ExtraCrashKeys = std::deque<crash_reporter::CrashKeyString<128>>;
ExtraCrashKeys& GetExtraCrashKeys() {
  static base::NoDestructor<ExtraCrashKeys> extra_keys;
  return *extra_keys;
}

std::deque<std::string>& GetExtraCrashKeyNames() {
  static base::NoDestructor<std::deque<std::string>> crash_key_names;
  return *crash_key_names;
}

}  // namespace

void SetCrashKey(const std::string& key, const std::string& value) {
  auto& crash_key_names = GetExtraCrashKeyNames();

  auto iter = std::find(crash_key_names.begin(), crash_key_names.end(), key);
  if (iter == crash_key_names.end()) {
    crash_key_names.emplace_back(key);
    GetExtraCrashKeys().emplace_back(crash_key_names.back().c_str());
    iter = crash_key_names.end() - 1;
  }
  GetExtraCrashKeys()[iter - crash_key_names.begin()].Set(value);
}

void ClearCrashKey(const std::string& key) {
  const auto& crash_key_names = GetExtraCrashKeyNames();

  auto iter = std::find(crash_key_names.begin(), crash_key_names.end(), key);
  if (iter != crash_key_names.end()) {
    GetExtraCrashKeys()[iter - crash_key_names.begin()].Clear();
  }
}

#if !defined(OS_LINUX)
void GetCrashKeys(std::map<std::string, std::string>* keys) {
  const auto& crash_key_names = GetExtraCrashKeyNames();
  const auto& crash_keys = GetExtraCrashKeys();
  int i = 0;
  for (const auto& key : crash_key_names) {
    const auto& value = crash_keys[i++];
    if (value.is_set()) {
      keys->emplace(key, value.value());
    }
  }
}
#endif

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

void SetPlatformCrashKey() {
  // TODO(nornagon): this is redundant with the 'plat' key that
  // //components/crash already includes. Remove it.
  static crash_reporter::CrashKeyString<8> platform_key("platform");
#if defined(OS_WIN)
  platform_key.Set("win32");
#elif defined(OS_MACOSX)
  platform_key.Set("darwin");
#elif defined(OS_LINUX)
  platform_key.Set("linux");
#else
  platform_key.Set("unknown");
#endif
}

}  // namespace crash_keys

}  // namespace electron
