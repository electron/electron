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
  SetUploadParameters();

  is_browser_ = upload_parameters_["process_type"].empty();
}

CrashReporter::~CrashReporter() {
}

void CrashReporter::Start(std::string product_name,
                          const std::string& company_name,
                          const std::string& submit_url,
                          bool auto_submit,
                          bool skip_system_crash_handler) {
  std::string version;
  if (is_browser_) {
    // Use application's version for crashes in browser.
    version = atom::Browser::Get()->GetVersion();
  } else {
    // Just use atom-shell's version in renderer, since we can not get
    // application's version here.
    version = ATOM_VERSION_STRING;

    // Append "Renderer" for the renderer.
    product_name += " Renderer";
  }

  InitBreakpad(product_name, version, company_name, submit_url, auto_submit,
               skip_system_crash_handler);
}

void CrashReporter::SetUploadParameters() {
  const CommandLine& command = *CommandLine::ForCurrentProcess();
  std::string type = command.GetSwitchValueASCII(switches::kProcessType);

  upload_parameters_["process_type"] = type;
  upload_parameters_["atom_shell_version"] = ATOM_VERSION_STRING;
}

}  // namespace crash_reporter
