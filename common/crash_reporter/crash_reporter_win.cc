// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/crash_reporter/crash_reporter_win.h"

#include "base/memory/singleton.h"

namespace crash_reporter {

CrashReporterWin::CrashReporterWin()
    : breakpad_(NULL) {
}

CrashReporterWin::~CrashReporterWin() {
  if (breakpad_ != NULL)
    BreakpadRelease(breakpad_);
}

void CrashReporterWin::InitBreakpad(const std::string& product_name,
                                    const std::string& version,
                                    const std::string& company_name,
                                    const std::string& submit_url,
                                    bool auto_submit,
                                    bool skip_system_crash_handler) {
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
