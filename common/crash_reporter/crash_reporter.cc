// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/crash_reporter/crash_reporter.h"

#include "base/command_line.h"
#include "browser/browser.h"
#include "common/atom_version.h"
#include "content/public/common/content_switches.h"

namespace crash_reporter {

CrashReporter::CrashReporter() {
  const CommandLine& command = *CommandLine::ForCurrentProcess();
  is_browser_ = command.GetSwitchValueASCII(switches::kProcessType).empty();
}

CrashReporter::~CrashReporter() {
}

void CrashReporter::Start(std::string product_name,
                          const std::string& company_name,
                          const std::string& submit_url,
                          bool auto_submit,
                          bool skip_system_crash_handler,
                          const StringMap& extra_parameters) {
  SetUploadParameters(extra_parameters);

  // Append "Renderer" for the renderer.
  product_name += " Renderer";
  InitBreakpad(product_name, ATOM_VERSION_STRING, company_name, submit_url,
               auto_submit, skip_system_crash_handler);
}

void CrashReporter::SetUploadParameters(const StringMap& parameters) {
  upload_parameters_ = parameters;
  upload_parameters_["process_type"] = is_browser_ ? "browser" : "renderer";
}

}  // namespace crash_reporter
