// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/crash_reporter/win/crash_service_main.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/logging.h"
#include "common/crash_reporter/win/crash_service.h"

namespace crash_service {

int Main(const wchar_t* cmd_line) {
  // Initialize all Chromium things.
  base::AtExitManager exit_manager;
  CommandLine::Init(0, NULL);

  VLOG(1) << "Session start. cmdline is [" << cmd_line << "]";

  wchar_t temp_dir[MAX_PATH] = { 0 };
  ::GetTempPathW(MAX_PATH, temp_dir);
  base::FilePath temp_path(temp_dir);

  breakpad::CrashService crash_service;
  if (!crash_service.Initialize(temp_path, temp_path))
    return 1;

  VLOG(1) << "Ready to process crash requests";

  // Enter the message loop.
  int retv = crash_service.ProcessingLoop();
  // Time to exit.
  VLOG(1) << "Session end. return code is " << retv;
  return retv;
}

}  // namespace crash_service
