// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/crash_reporter/win/crash_service_main.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/logging.h"
#include "common/crash_reporter/win/crash_service.h"

namespace crash_service {

namespace {

const wchar_t kStandardLogFile[] = L"operation_log.txt";

bool GetCrashServiceDirectory(base::FilePath* dir) {
  base::FilePath temp_dir;
  if (!file_util::GetTempDir(&temp_dir))
    return false;
  temp_dir = temp_dir.Append(L"atom_crashes");
  if (!file_util::PathExists(temp_dir)) {
    if (!file_util::CreateDirectory(temp_dir))
      return false;
  }
  *dir = temp_dir;
  return true;
}

}  // namespace.

int Main(const wchar_t* cmd_line) {
  // Initialize all Chromium things.
  base::AtExitManager exit_manager;
  CommandLine::Init(0, NULL);

  // We use/create a directory under the user's temp folder, for logging.
  base::FilePath operating_dir;
  GetCrashServiceDirectory(&operating_dir);
  base::FilePath log_file = operating_dir.Append(kStandardLogFile);

  // Logging out to a file.
  logging::InitLogging(
      log_file.value().c_str(),
      logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG ,
      logging::LOCK_LOG_FILE,
      logging::DELETE_OLD_LOG_FILE,
      logging::DISABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS);
  logging::SetLogItems(true, false, true, false);

  VLOG(1) << "Session start. cmdline is [" << cmd_line << "]";

  breakpad::CrashService crash_service;
  if (!crash_service.Initialize(operating_dir, operating_dir))
    return 1;

  VLOG(1) << "Ready to process crash requests";

  // Enter the message loop.
  int retv = crash_service.ProcessingLoop();
  // Time to exit.
  VLOG(1) << "Session end. return code is " << retv;
  return retv;
}

}  // namespace crash_service
