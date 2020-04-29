// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/app/electron_crash_reporter_client.h"

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/common/chrome_paths.h"
#include "components/crash/core/common/crash_keys.h"
#include "components/upload_list/crash_upload_list.h"
#include "content/public/common/content_switches.h"
#include "electron/electron_version.h"
#include "services/service_manager/embedder/switches.h"

#if defined(OS_POSIX) && !defined(OS_MACOSX)
#include "components/version_info/version_info_values.h"
#endif

#if defined(OS_POSIX)
#include "base/debug/dump_without_crashing.h"
#endif

void ElectronCrashReporterClient::Create() {
  static base::NoDestructor<ElectronCrashReporterClient> crash_client;
  crash_reporter::SetCrashReporterClient(crash_client.get());

  // By setting the BREAKPAD_DUMP_LOCATION environment variable, an alternate
  // location to write crash dumps can be set.
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string alternate_crash_dump_location;
  base::FilePath crash_dumps_dir_path;
  if (env->GetVar("BREAKPAD_DUMP_LOCATION", &alternate_crash_dump_location)) {
    crash_dumps_dir_path =
        base::FilePath::FromUTF8Unsafe(alternate_crash_dump_location);
  }
  if (!crash_dumps_dir_path.empty()) {
    base::PathService::Override(chrome::DIR_CRASH_DUMPS, crash_dumps_dir_path);
  }
}

ElectronCrashReporterClient::ElectronCrashReporterClient() {}

ElectronCrashReporterClient::~ElectronCrashReporterClient() {}

#if !defined(OS_MACOSX)
void ElectronCrashReporterClient::SetCrashReporterClientIdFromGUID(
    const std::string& client_guid) {
  crash_keys::SetMetricsClientIdFromGUID(client_guid);
}
#endif

#if defined(OS_POSIX) && !defined(OS_MACOSX)
void ElectronCrashReporterClient::GetProductNameAndVersion(
    const char** product_name,
    const char** version) {
  DCHECK(product_name);
  DCHECK(version);
  *product_name = ELECTRON_PRODUCT_NAME;
  *version = ELECTRON_VERSION_STRING;
}

void ElectronCrashReporterClient::GetProductNameAndVersion(
    std::string* product_name,
    std::string* version,
    std::string* channel) {
  const char* c_product_name;
  const char* c_version;
  GetProductNameAndVersion(&c_product_name, &c_version);
  *product_name = c_product_name;
  *version = c_version;
  *channel = "";
}

base::FilePath ElectronCrashReporterClient::GetReporterLogFilename() {
  return base::FilePath(CrashUploadList::kReporterLogFilename);
}
#endif

bool ElectronCrashReporterClient::GetCrashDumpLocation(
    base::FilePath* crash_dir) {
  return base::PathService::Get(chrome::DIR_CRASH_DUMPS, crash_dir);
}

#if defined(OS_MACOSX) || defined(OS_LINUX)
bool ElectronCrashReporterClient::GetCrashMetricsLocation(
    base::FilePath* metrics_dir) {
  return base::PathService::Get(chrome::DIR_USER_DATA, metrics_dir);
}
#endif  // OS_MACOSX || OS_LINUX

bool ElectronCrashReporterClient::IsRunningUnattended() {
  return false;
}

bool ElectronCrashReporterClient::GetCollectStatsConsent() {
  return true;
}

#if defined(OS_LINUX)
bool ElectronCrashReporterClient::ShouldMonitorCrashHandlerExpensively() {
  return false;
}
#endif  // OS_LINUX

bool ElectronCrashReporterClient::EnableBreakpadForProcess(
    const std::string& process_type) {
  return process_type == switches::kRendererProcess ||
         process_type == switches::kPpapiPluginProcess ||
         process_type == service_manager::switches::kZygoteProcess ||
         process_type == switches::kGpuProcess ||
         process_type == switches::kUtilityProcess;
}
