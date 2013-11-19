// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/crash_reporter/crash_reporter_win.h"

#include "base/logging.h"
#include "base/memory/singleton.h"

namespace crash_reporter {

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

  wchar_t temp_dir[MAX_PATH] = { 0 };
  ::GetTempPathW(MAX_PATH, temp_dir);

  breakpad_.reset(new google_breakpad::ExceptionHandler(temp_dir,
      FilterCallback,
      MinidumpCallback,
      this,
      google_breakpad::ExceptionHandler::HANDLER_ALL));
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
