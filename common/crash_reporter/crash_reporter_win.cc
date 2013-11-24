// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/crash_reporter/crash_reporter_win.h"

#include "base/file_util.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"

namespace crash_reporter {

namespace {

// Minidump with stacks, PEB, TEB, and unloaded module list.
const MINIDUMP_TYPE kSmallDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithProcessThreadData |  // Get PEB and TEB.
    MiniDumpWithUnloadedModules);  // Get unloaded modules when available.

const wchar_t kPipeNameFormat[] = L"\\\\.\\pipe\\$1 Crash Service";

}  // namespace

CrashReporterWin::CrashReporterWin() {
}

CrashReporterWin::~CrashReporterWin() {
}

void CrashReporterWin::InitBreakpad(const std::string& product_name,
                                    const std::string& version,
                                    const std::string& company_name,
                                    const std::string& submit_url,
                                    bool auto_submit,
                                    bool skip_system_crash_handler) {
  skip_system_crash_handler_ = skip_system_crash_handler;

  base::FilePath temp_dir;
  if (!file_util::GetTempDir(&temp_dir)) {
    LOG(ERROR) << "Cannot get temp directory";
    return;
  }

  string16 pipe_name = ReplaceStringPlaceholders(kPipeNameFormat,
                                                 UTF8ToUTF16(product_name),
                                                 NULL);

  // Wait until the crash service is started.
  HANDLE waiting_event =
      ::CreateEventW(NULL, TRUE, FALSE, L"g_atom_shell_crash_service");
  if (waiting_event != INVALID_HANDLE_VALUE)
    WaitForSingleObject(waiting_event, 1000);

  google_breakpad::CustomClientInfo custom_info = { NULL, 0 };
  breakpad_.reset(new google_breakpad::ExceptionHandler(
      temp_dir.value(),
      FilterCallback,
      MinidumpCallback,
      this,
      google_breakpad::ExceptionHandler::HANDLER_ALL,
      kSmallDumpType,
      pipe_name.c_str(),
      &custom_info));

  if (!breakpad_->IsOutOfProcess())
    LOG(ERROR) << "Cannot initialize out-of-process crash handler";
}

void CrashReporterWin::SetUploadParameters() {
  upload_parameters_["platform"] = "win32";
}

// static
bool CrashReporterWin::FilterCallback(void* context,
                                      EXCEPTION_POINTERS* exinfo,
                                      MDRawAssertionInfo* assertion) {
  return true;
}

// static
bool CrashReporterWin::MinidumpCallback(const wchar_t* dump_path,
                                        const wchar_t* minidump_id,
                                        void* context,
                                        EXCEPTION_POINTERS* exinfo,
                                        MDRawAssertionInfo* assertion,
                                        bool succeeded) {
  CrashReporterWin* self = static_cast<CrashReporterWin*>(context);
  if (succeeded && !self->skip_system_crash_handler_)
    return true;
  else
    return false;
}

// static
CrashReporterWin* CrashReporterWin::GetInstance() {
  return Singleton<CrashReporterWin>::get();
}

// static
CrashReporter* CrashReporter::GetInstance() {
  return CrashReporterWin::GetInstance();
}

}  // namespace crash_reporter
