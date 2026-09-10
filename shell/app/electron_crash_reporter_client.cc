// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/app/electron_crash_reporter_client.h"

#include <map>
#include <string>

#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/crash/core/common/crash_keys.h"
#include "components/upload_list/crash_upload_list.h"
#include "content/public/common/content_switches.h"
#include "electron/electron_version.h"
#include "shell/common/electron_paths.h"
#include "shell/common/thread_restrictions.h"

#if BUILDFLAG(IS_POSIX) && !BUILDFLAG(IS_MAC)
#include "components/version_info/version_info_values.h"
#endif

#if BUILDFLAG(IS_WIN)
#include <windows.h>

#include <vector>

#include "base/base_paths.h"
#include "base/strings/strcat_win.h"
#include "base/strings/string_util_win.h"
#include "base/win/registry.h"
#endif

namespace {

ElectronCrashReporterClient* Instance() {
  static base::NoDestructor<ElectronCrashReporterClient> crash_client;
  return crash_client.get();
}

}  // namespace

// static
void ElectronCrashReporterClient::Create() {
  crash_reporter::SetCrashReporterClient(Instance());

  // By setting the BREAKPAD_DUMP_LOCATION environment variable, an alternate
  // location to write crash dumps can be set.
  auto env = base::Environment::Create();
  base::FilePath crash_dumps_dir_path;
  if (std::optional<std::string> alternate_crash_dump_location =
          env->GetVar("BREAKPAD_DUMP_LOCATION")) {
    crash_dumps_dir_path =
        base::FilePath::FromUTF8Unsafe(alternate_crash_dump_location.value());
  }
  if (!crash_dumps_dir_path.empty()) {
    electron::ScopedAllowBlockingForElectron allow_blocking;
    base::PathService::Override(electron::DIR_CRASH_DUMPS,
                                crash_dumps_dir_path);
  }
}

// static
ElectronCrashReporterClient* ElectronCrashReporterClient::Get() {
  return Instance();
}

void ElectronCrashReporterClient::SetCollectStatsConsent(bool upload_allowed) {
  collect_stats_consent_ = upload_allowed;
}

void ElectronCrashReporterClient::SetUploadUrl(const std::string& url) {
  upload_url_ = url;
}

void ElectronCrashReporterClient::SetShouldRateLimit(bool rate_limit) {
  rate_limit_ = rate_limit;
}

void ElectronCrashReporterClient::SetShouldCompressUploads(bool compress) {
  compress_uploads_ = compress;
}

void ElectronCrashReporterClient::SetGlobalAnnotations(
    const std::map<std::string, std::string>& annotations) {
  global_annotations_ = annotations;
}

ElectronCrashReporterClient::ElectronCrashReporterClient() = default;

ElectronCrashReporterClient::~ElectronCrashReporterClient() = default;

#if BUILDFLAG(IS_LINUX)
void ElectronCrashReporterClient::SetCrashReporterClientIdFromGUID(
    const std::string& client_guid) {
  crash_keys::SetMetricsClientIdFromGUID(client_guid);
}

base::FilePath ElectronCrashReporterClient::GetReporterLogFilename() {
  return base::FilePath(CrashUploadList::kReporterLogFilename);
}
#endif

#if BUILDFLAG(IS_WIN)
namespace {

// Copied from crashpad's crashpad_wer.dll by //electron:electron_wer and
// shipped in the assets directory (the executable's directory by default) as
// <exe name>_wer.dll, so an app that renames electron.exe to myapp.exe must
// rename the helper to myapp_wer.dll.
constexpr base::FilePath::CharType kWerHelperSuffix[] =
    FILE_PATH_LITERAL("_wer.dll");

// Windows Error Reporting only loads runtime exception helper modules that
// are listed (by full path, as a value name) under this key in HKCU or HKLM.
constexpr wchar_t kWerHelperRegistryKey[] =
    L"Software\\Microsoft\\Windows\\Windows Error Reporting"
    L"\\RuntimeExceptionHelperModules";

base::FilePath GetWerHelperPath() {
  base::FilePath exe, assets_dir;
  if (!base::PathService::Get(base::FILE_EXE, &exe) ||
      !base::PathService::Get(base::DIR_ASSETS, &assets_dir)) {
    return {};
  }
  return assets_dir.Append(base::StrCat(
      {exe.BaseName().RemoveExtension().value(), kWerHelperSuffix}));
}

}  // namespace

void ElectronCrashReporterClient::GetProductNameAndVersion(
    const std::wstring& exe_path,
    std::wstring* product_name,
    std::wstring* version,
    std::wstring* special_build,
    std::wstring* channel_name) {
  *product_name = base::UTF8ToWide(ELECTRON_PRODUCT_NAME);
  *version = base::UTF8ToWide(ELECTRON_VERSION_STRING);
}

std::wstring ElectronCrashReporterClient::GetWerRuntimeExceptionModule() {
  // Called once per process during crashpad initialization, including in
  // sandboxed children, so do not touch the disk here; registering a path
  // that does not exist is harmless (WER only loads listed, existing DLLs).
  return GetWerHelperPath().value();
}

// static
void ElectronCrashReporterClient::RegisterWerHelperModuleForCurrentUser() {
  electron::ScopedAllowBlockingForElectron allow_blocking;
  const base::FilePath path = GetWerHelperPath();
  if (path.empty() || !base::PathExists(path))
    return;

  base::win::RegKey key;
  if (key.Create(HKEY_CURRENT_USER, kWerHelperRegistryKey,
                 KEY_QUERY_VALUE | KEY_SET_VALUE) != ERROR_SUCCESS) {
    return;
  }

  // Installers that version the install directory (e.g. Squirrel's
  // app-x.y.z folders) leave one value behind per update. Prune entries for
  // sibling copies of the helper under this app's install root that no longer
  // exist on disk; other apps' entries are left alone.
  const base::FilePath install_root = path.DirName().DirName();
  std::vector<std::wstring> stale;
  for (base::win::RegistryValueIterator it(HKEY_CURRENT_USER,
                                           kWerHelperRegistryKey);
       it.Valid(); ++it) {
    base::FilePath registered(it.Name());
    if (registered != path &&
        base::FilePath::CompareEqualIgnoreCase(registered.BaseName().value(),
                                               path.BaseName().value()) &&
        base::FilePath::CompareEqualIgnoreCase(
            registered.DirName().DirName().value(), install_root.value()) &&
        !base::PathExists(registered)) {
      stale.emplace_back(it.Name());
    }
  }
  for (const std::wstring& name : stale)
    key.DeleteValue(name.c_str());

  if (!key.HasValue(path.value().c_str())) {
    // The value's data is ignored by WER; only the name matters.
    key.WriteValue(path.value().c_str(), DWORD{0});
  }
}
#endif

#if BUILDFLAG(IS_WIN)
bool ElectronCrashReporterClient::GetCrashDumpLocation(
    std::wstring* crash_dir_str) {
  base::FilePath crash_dir;
  if (!base::PathService::Get(electron::DIR_CRASH_DUMPS, &crash_dir))
    return false;
  *crash_dir_str = crash_dir.value();
  return true;
}
#else
bool ElectronCrashReporterClient::GetCrashDumpLocation(
    base::FilePath* crash_dir) {
  bool result = base::PathService::Get(electron::DIR_CRASH_DUMPS, crash_dir);
  {
    // If the DIR_CRASH_DUMPS path is overridden with
    // app.setPath('crashDumps', ...) then the directory might not have been
    // created.
    electron::ScopedAllowBlockingForElectron allow_blocking;
    if (result && !base::PathExists(*crash_dir)) {
      return base::CreateDirectory(*crash_dir);
    }
  }
  return result;
}
#endif

bool ElectronCrashReporterClient::IsRunningUnattended() {
  return !collect_stats_consent_;
}

bool ElectronCrashReporterClient::GetCollectStatsConsent() {
  return collect_stats_consent_;
}

#if BUILDFLAG(IS_MAC)
bool ElectronCrashReporterClient::ReportingIsEnforcedByPolicy(
    bool* breakpad_enabled) {
  return false;
}
#endif

bool ElectronCrashReporterClient::GetShouldRateLimit() {
  return rate_limit_;
}

bool ElectronCrashReporterClient::GetShouldCompressUploads() {
  return compress_uploads_;
}

void ElectronCrashReporterClient::GetProcessSimpleAnnotations(
    std::map<std::string, std::string>* annotations) {
  for (auto&& pair : global_annotations_) {
    (*annotations)[pair.first] = pair.second;
  }
  (*annotations)["prod"] = ELECTRON_PRODUCT_NAME;
  (*annotations)["ver"] = ELECTRON_VERSION_STRING;
}

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
bool ElectronCrashReporterClient::ShouldMonitorCrashHandlerExpensively() {
  return false;
}
#endif

std::string ElectronCrashReporterClient::GetUploadUrl() {
  return upload_url_;
}

void ElectronCrashReporterClient::GetProductInfo(ProductInfo* product_info) {
  product_info->product_name = ELECTRON_PRODUCT_NAME;
  product_info->version = ELECTRON_VERSION_STRING;
}

bool ElectronCrashReporterClient::EnableBreakpadForProcess(
    const std::string& process_type) {
  return process_type == switches::kRendererProcess ||
         process_type == switches::kZygoteProcess ||
         process_type == switches::kGpuProcess ||
         process_type == switches::kUtilityProcess || process_type == "node";
}
