// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/crash_reporter/crash_reporter_win.h"

#include <memory>

#include "base/environment.h"
#include "base/memory/singleton.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "crashpad/client/crashpad_client.h"
#include "crashpad/client/crashpad_info.h"
#if defined(_WIN64)
#include "gin/public/debug.h"
#endif
namespace {

#if defined(_WIN64)
int CrashForException(EXCEPTION_POINTERS* info) {
  auto* reporter = crash_reporter::CrashReporterWin::GetInstance();
  if (reporter->IsInitialized())
    reporter->GetCrashpadClient().DumpAndCrash(info);
  return EXCEPTION_CONTINUE_SEARCH;
}
#endif
const char kPipeNameVar[] = "ELECTRON_CRASHPAD_PIPE_NAME";

}  // namespace

namespace crash_reporter {

CrashReporterWin::CrashReporterWin() {}

CrashReporterWin::~CrashReporterWin() {}

#if defined(_WIN64)
void CrashReporterWin::SetUnhandledExceptionFilter() {
  gin::Debug::SetUnhandledExceptionCallback(&CrashForException);
}
#endif

void CrashReporterWin::InitBreakpad(const std::string& product_name,
                                    const std::string& version,
                                    const std::string& company_name,
                                    const std::string& submit_url,
                                    const base::FilePath& crashes_dir,
                                    bool upload_to_server,
                                    bool skip_system_crash_handler) {
  // check whether crashpad has been initialized.
  // Only need to initialize once.
  if (simple_string_dictionary_)
    return;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (is_browser_) {
    base::FilePath handler_path;
    base::PathService::Get(base::FILE_EXE, &handler_path);

    std::vector<std::string> args = {
        "--no-rate-limit",
        "--no-upload-gzip",  // not all servers accept gzip
    };
    std::string process_arg = "--type=$1";
    args.push_back(
        base::ReplaceStringPlaceholders(process_arg, {kCrashpadProcess}, NULL));
    std::string crashes_dir_arg = "--$1=$2";
    args.push_back(base::ReplaceStringPlaceholders(
        crashes_dir_arg,
        {kCrashesDirectoryKey, base::UTF16ToASCII(crashes_dir.value())}, NULL));
    crashpad_client_.StartHandler(handler_path, crashes_dir, crashes_dir,
                                  submit_url, StringMap(), args, true, false);
    env->SetVar(kPipeNameVar,
                base::UTF16ToUTF8(GetCrashpadClient().GetHandlerIPCPipe()));
  } else {
    std::string pipe_name_utf8;
    if (env->GetVar(kPipeNameVar, &pipe_name_utf8)) {
      crashpad_client_.SetHandlerIPCPipe(base::UTF8ToUTF16(pipe_name_utf8));
    }
  }
  crashpad::CrashpadInfo* crashpad_info =
      crashpad::CrashpadInfo::GetCrashpadInfo();
  if (skip_system_crash_handler) {
    crashpad_info->set_system_crash_reporter_forwarding(
        crashpad::TriState::kDisabled);
  }
  simple_string_dictionary_.reset(new crashpad::SimpleStringDictionary());
  crashpad_info->set_simple_annotations(simple_string_dictionary_.get());

  SetInitialCrashKeyValues(version);
  if (is_browser_) {
    database_ = crashpad::CrashReportDatabase::Initialize(crashes_dir);
    SetUploadToServer(upload_to_server);
  }
}

void CrashReporterWin::SetUploadParameters() {
  upload_parameters_["platform"] = "win32";
}

crashpad::CrashpadClient& CrashReporterWin::GetCrashpadClient() {
  return crashpad_client_;
}

// static
CrashReporterWin* CrashReporterWin::GetInstance() {
  return base::Singleton<CrashReporterWin>::get();
}

// static
CrashReporter* CrashReporter::GetInstance() {
  return CrashReporterWin::GetInstance();
}

}  // namespace crash_reporter
