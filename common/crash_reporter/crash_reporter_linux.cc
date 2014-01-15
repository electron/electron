// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/crash_reporter/crash_reporter_linux.h"

#include "base/memory/singleton.h"

namespace crash_reporter {

CrashReporterLinux::CrashReporterLinux() {
}

CrashReporterLinux::~CrashReporterLinux() {
}

void CrashReporterLinux::InitBreakpad(const std::string& product_name,
                                    const std::string& version,
                                    const std::string& company_name,
                                    const std::string& submit_url,
                                    bool auto_submit,
                                    bool skip_system_crash_handler) {
}

void CrashReporterLinux::SetUploadParameters() {
}

// static
CrashReporterLinux* CrashReporterLinux::GetInstance() {
  return Singleton<CrashReporterLinux>::get();
}

// static
CrashReporter* CrashReporter::GetInstance() {
  return CrashReporterLinux::GetInstance();
}

}  // namespace crash_reporter
