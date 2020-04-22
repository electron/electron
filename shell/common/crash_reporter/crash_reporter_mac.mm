// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/crash_reporter/crash_reporter_mac.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/mac_util.h"
#include "base/memory/singleton.h"
#include "content/public/common/content_switches.h"
#include "electron/buildflags/buildflags.h"
#include "electron/electron_version.h"
#include "shell/common/electron_constants.h"
#include "third_party/crashpad/crashpad/client/crashpad_client.h"
#include "third_party/crashpad/crashpad/client/crashpad_info.h"

namespace crash_reporter {

CrashReporterMac::CrashReporterMac() {}

CrashReporterMac::~CrashReporterMac() {}

void CrashReporterMac::Init(const std::string& submit_url,
                            const base::FilePath& crashes_dir,
                            bool upload_to_server,
                            bool skip_system_crash_handler,
                            bool rate_limit,
                            bool compress,
                            const StringMap& global_extra_parameters) {
  // check whether crashpad has been initialized.
  // Only need to initialize once.
  if (simple_string_dictionary_)
    return;

  if (process_type_ == "node") {
    // TODO(jeremy): we should differentiate here between the node process
    // being a child process (i.e. forked from an Electron process which has
    // the crash reporter enabled) and being an _initial_ process in which we
    // should boot the crash reporter from scratch.
    InitInChild();
    return;
  }

  DCHECK(process_type_.empty());
  if (process_type_.empty()) {  // browser process
    @autoreleasepool {
      base::FilePath framework_bundle_path = base::mac::FrameworkBundlePath();
      base::FilePath handler_path =
          framework_bundle_path.Append("Resources").Append("crashpad_handler");

      // These are static annotations, which are required by breakpad-type
      // servers, so set them as app-wide annotations.
      // TODO(jeremy): should we put the app name/version here as well?
      std::map<std::string, std::string> annotations;
      annotations.emplace("prod", ELECTRON_PRODUCT_NAME);
      annotations.emplace("ver", ELECTRON_VERSION_STRING);
      for (const auto& i : global_extra_parameters) {
        annotations.insert(i);
      }

      std::vector<std::string> args;
      if (!rate_limit)
        args.emplace_back("--no-rate-limit");
      if (!compress)
        args.emplace_back("--no-upload-gzip");

      crashpad::CrashpadClient crashpad_client;
      crashpad_client.StartHandler(handler_path, crashes_dir, crashes_dir,
                                   submit_url, annotations, args, true, false);
    }  // @autoreleasepool
  }

  crashpad::CrashpadInfo* crashpad_info =
      crashpad::CrashpadInfo::GetCrashpadInfo();
  if (skip_system_crash_handler) {
    crashpad_info->set_system_crash_reporter_forwarding(
        crashpad::TriState::kDisabled);
  }

  simple_string_dictionary_.reset(new crashpad::SimpleStringDictionary());
  crashpad_info->set_simple_annotations(simple_string_dictionary_.get());

  SetInitialCrashKeyValues();
  if (process_type_.empty()) {  // browser process
    database_ = crashpad::CrashReportDatabase::Initialize(crashes_dir);
    SetUploadToServer(upload_to_server);
  }
}

void CrashReporterMac::InitInChild() {
  if (simple_string_dictionary_)
    return;

  crashpad::CrashpadInfo* crashpad_info =
      crashpad::CrashpadInfo::GetCrashpadInfo();

  // Always disable system crash reporter in child processes, as we don't want
  // to see a system crash dialog for renderer crashes.
  crashpad_info->set_system_crash_reporter_forwarding(
      crashpad::TriState::kDisabled);
  simple_string_dictionary_.reset(new crashpad::SimpleStringDictionary());
  crashpad_info->set_simple_annotations(simple_string_dictionary_.get());

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  bool run_as_node = base::Environment::Create()->HasVar(electron::kRunAsNode);
#else
  bool run_as_node = false;
#endif

  std::string process_type;

  if (run_as_node) {
    process_type = "node";
  } else {
    auto* command_line = base::CommandLine::ForCurrentProcess();
    process_type = command_line->GetSwitchValueASCII(switches::kProcessType);
  }

  SetCrashKeyValue("process_type", process_type);
  SetCrashKeyValue("platform", "darwin");
}

void CrashReporterMac::SetUploadParameters() {
  upload_parameters_["platform"] = "darwin";
}

// static
CrashReporterMac* CrashReporterMac::GetInstance() {
  return base::Singleton<CrashReporterMac>::get();
}

// static
CrashReporter* CrashReporter::GetInstance() {
  return CrashReporterMac::GetInstance();
}

}  // namespace crash_reporter
