// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/crash_reporter/crash_reporter_mac.h"

#include "base/files/file_path.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/mac_util.h"
#include "base/memory/singleton.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "vendor/crashpad/client/crash_report_database.h"
#include "vendor/crashpad/client/crashpad_client.h"
#include "vendor/crashpad/client/crashpad_info.h"
#include "vendor/crashpad/client/settings.h"
#include "vendor/crashpad/client/simple_string_dictionary.h"

namespace crash_reporter {

namespace {

crashpad::SimpleStringDictionary* g_simple_string_dictionary;
crashpad::CrashReportDatabase* g_database;

void SetUploadsEnabled(bool enable_uploads) {
  if (g_database) {
    crashpad::Settings* settings = g_database->GetSettings();
    settings->SetUploadsEnabled(enable_uploads);
  }
}

void SetCrashKeyValue(const base::StringPiece& key,
                      const base::StringPiece& value) {
  g_simple_string_dictionary->SetKeyValue(key.data(), value.data());
}

}  // namespace

CrashReporterMac::CrashReporterMac() {
}

CrashReporterMac::~CrashReporterMac() {
}

void CrashReporterMac::InitBreakpad(const std::string& product_name,
                                    const std::string& version,
                                    const std::string& company_name,
                                    const std::string& submit_url,
                                    bool auto_submit,
                                    bool skip_system_crash_handler) {
  static bool initialized = false;
  if (initialized)
    return;
  initialized = true;

  std::string dump_dir = "/tmp/" + product_name + " Crashes";
  base::FilePath database_path(dump_dir);
  if (is_browser_) {
    @autoreleasepool {
      base::FilePath framework_bundle_path = base::mac::FrameworkBundlePath();
      base::FilePath handler_path =
          framework_bundle_path.Append("Resources").Append("crashpad_handler");

      crashpad::CrashpadClient crashpad_client;
      if (crashpad_client.StartHandler(handler_path, database_path,
                                       submit_url,
                                       StringMap(),
                                       std::vector<std::string>())) {
        crashpad_client.UseHandler();
      }
    }  // @autoreleasepool
  }

  crashpad::CrashpadInfo* crashpad_info =
      crashpad::CrashpadInfo::GetCrashpadInfo();
  if (skip_system_crash_handler) {
    crashpad_info->set_system_crash_reporter_forwarding(
        crashpad::TriState::kDisabled);
  }

  g_simple_string_dictionary = new crashpad::SimpleStringDictionary();
  crashpad_info->set_simple_annotations(g_simple_string_dictionary);

  SetCrashKeyValue("prod", ATOM_PRODUCT_NAME);
  SetCrashKeyValue("process_type", is_browser_ ? base::StringPiece("browser")
                                               : base::StringPiece("renderer"));
  SetCrashKeyValue("ver", version);

  for (auto iter = upload_parameters_.begin();
       iter != upload_parameters_.end(); ++iter) {
    SetCrashKeyValue(iter->first, iter->second);
  }
  if (is_browser_) {
    g_database =
        crashpad::CrashReportDatabase::Initialize(database_path).release();
    SetUploadsEnabled(auto_submit);
  }
}

void CrashReporterMac::SetUploadParameters() {
  upload_parameters_["platform"] = "darwin";
}

// static
CrashReporterMac* CrashReporterMac::GetInstance() {
  return Singleton<CrashReporterMac>::get();
}

// static
CrashReporter* CrashReporter::GetInstance() {
  return CrashReporterMac::GetInstance();
}

}  // namespace crash_reporter
