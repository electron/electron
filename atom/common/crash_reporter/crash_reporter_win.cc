// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/crash_reporter/crash_reporter_win.h"

#include <string>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

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
  if (!base::GetTempDir(&temp_dir)) {
    LOG(ERROR) << "Cannot get temp directory";
    return;
  }

  base::string16 pipe_name = ReplaceStringPlaceholders(
      kPipeNameFormat, base::UTF8ToUTF16(product_name), NULL);

  // Wait until the crash service is started.
  HANDLE waiting_event =
      ::CreateEventW(NULL, TRUE, FALSE, L"g_atom_shell_crash_service");
  if (waiting_event != INVALID_HANDLE_VALUE)
    WaitForSingleObject(waiting_event, 1000);

  // ExceptionHandler() attaches our handler and ~ExceptionHandler() detaches
  // it, so we must explicitly reset *before* we instantiate our new handler
  // to allow any previous handler to detach in the correct order.
  breakpad_.reset();

  int handler_types = google_breakpad::ExceptionHandler::HANDLER_EXCEPTION |
      google_breakpad::ExceptionHandler::HANDLER_PURECALL;
  breakpad_.reset(new google_breakpad::ExceptionHandler(
      temp_dir.value(),
      FilterCallback,
      MinidumpCallback,
      this,
      handler_types,
      kSmallDumpType,
      pipe_name.c_str(),
      GetCustomInfo(product_name, version, company_name)));

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

google_breakpad::CustomClientInfo* CrashReporterWin::GetCustomInfo(
    const std::string& product_name,
    const std::string& version,
    const std::string& company_name) {
  custom_info_entries_.clear();
  custom_info_entries_.reserve(2 + upload_parameters_.size());

  custom_info_entries_.push_back(google_breakpad::CustomInfoEntry(
      L"prod", L"Electron"));
  custom_info_entries_.push_back(google_breakpad::CustomInfoEntry(
      L"ver", base::UTF8ToWide(version).c_str()));

  for (StringMap::const_iterator iter = upload_parameters_.begin();
       iter != upload_parameters_.end(); ++iter) {
    custom_info_entries_.push_back(google_breakpad::CustomInfoEntry(
        base::UTF8ToWide(iter->first).c_str(),
        base::UTF8ToWide(iter->second).c_str()));
  }

  custom_info_.entries = &custom_info_entries_.front();
  custom_info_.count = custom_info_entries_.size();
  return &custom_info_;
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
