// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/crash_keys.h"

#include <deque>
#include <map>
#include <string>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/no_destructor.h"
#include "base/strings/string_split.h"
#include "components/crash/core/common/crash_key.h"
#include "content/public/common/content_switches.h"
#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/electron_constants.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "shell/common/process_util.h"
#include "third_party/crashpad/crashpad/client/annotation.h"

namespace electron::crash_keys {

namespace {

constexpr size_t kMaxCrashKeyValueSize = 20320;
static_assert(kMaxCrashKeyValueSize < crashpad::Annotation::kValueMaxSize,
              "max crash key value length above what crashpad supports");

using ExtraCrashKeys =
    std::deque<crash_reporter::CrashKeyString<kMaxCrashKeyValueSize>>;
ExtraCrashKeys& GetExtraCrashKeys() {
  static base::NoDestructor<ExtraCrashKeys> extra_keys;
  return *extra_keys;
}

std::deque<std::string>& GetExtraCrashKeyNames() {
  static base::NoDestructor<std::deque<std::string>> crash_key_names;
  return *crash_key_names;
}

}  // namespace

constexpr uint32_t kMaxCrashKeyNameLength = 40;
static_assert(kMaxCrashKeyNameLength <= crashpad::Annotation::kNameMaxLength,
              "max crash key name length above what crashpad supports");

void SetCrashKey(const std::string& key, const std::string& value) {
  // Chrome DCHECK()s if we try to set an annotation with a name longer than
  // the max.
  if (key.size() >= kMaxCrashKeyNameLength) {
    node::Environment* env =
        node::Environment::GetCurrent(JavascriptEnvironment::GetIsolate());
    EmitWarning(env,
                "The crash key name, \"" + key + "\", is longer than " +
                    std::to_string(kMaxCrashKeyNameLength) +
                    " bytes, ignoring it.",
                "electron");
    return;
  }

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

namespace {
bool IsRunningAsNode() {
  return electron::fuses::IsRunAsNodeEnabled() &&
         base::Environment::Create()->HasVar(electron::kRunAsNode);
}
}  // namespace

void SetCrashKeysFromCommandLine(const base::CommandLine& command_line) {
  // NB. this is redundant with the 'ptype' key that //components/crash
  // reports; it's present for backwards compatibility.
  static crash_reporter::CrashKeyString<16> process_type_key("process_type");
  if (IsRunningAsNode()) {
    process_type_key.Set("node");
  } else {
    std::string process_type =
        command_line.GetSwitchValueASCII(::switches::kProcessType);
    if (process_type.empty()) {
      process_type_key.Set("browser");
    } else {
      process_type_key.Set(process_type);
    }
  }
}

void SetPlatformCrashKey() {
  // TODO(nornagon): this is redundant with the 'plat' key that
  // //components/crash already includes. Remove it.
  static crash_reporter::CrashKeyString<8> platform_key("platform");
#if BUILDFLAG(IS_WIN)
  platform_key.Set("win32");
#elif BUILDFLAG(IS_MAC)
  platform_key.Set("darwin");
#elif BUILDFLAG(IS_LINUX)
  platform_key.Set("linux");
#else
  platform_key.Set("unknown");
#endif
}

}  // namespace electron::crash_keys
