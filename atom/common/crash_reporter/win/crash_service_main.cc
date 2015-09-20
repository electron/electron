// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/crash_reporter/win/crash_service_main.h"

#include "atom/common/crash_reporter/win/crash_service.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"

namespace crash_service {

namespace {

const char kApplicationName[] = "application-name";

const wchar_t kPipeNameFormat[] = L"\\\\.\\pipe\\$1 Crash Service";
const wchar_t kStandardLogFile[] = L"operation_log.txt";

bool GetCrashServiceDirectory(const std::wstring& application_name,
                              base::FilePath* dir) {
  base::FilePath temp_dir;
  if (!base::GetTempDir(&temp_dir))
    return false;
  temp_dir = temp_dir.Append(application_name + L" Crashes");
  if (!base::PathExists(temp_dir)) {
    if (!base::CreateDirectory(temp_dir))
      return false;
  }
  *dir = temp_dir;
  return true;
}

}  // namespace.

int Main(const wchar_t* cmd) {
  // Initialize all Chromium things.
  base::AtExitManager exit_manager;
  base::CommandLine::Init(0, NULL);
  base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();

  // Use the application's name as pipe name and output directory.
  if (!cmd_line.HasSwitch(kApplicationName)) {
    LOG(ERROR) << "Application's name must be specified with --"
               << kApplicationName;
    return 1;
  }
  std::wstring application_name = cmd_line.GetSwitchValueNative(
      kApplicationName);

  // We use/create a directory under the user's temp folder, for logging.
  base::FilePath operating_dir;
  GetCrashServiceDirectory(application_name, &operating_dir);
  base::FilePath log_file = operating_dir.Append(kStandardLogFile);

  // Logging to stderr (to help with debugging failures on the
  // buildbots) and to a file.
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_ALL;
  settings.log_file = log_file.value().c_str();
  logging::InitLogging(settings);
  // Logging with pid, tid and timestamp.
  logging::SetLogItems(true, true, true, false);

  VLOG(1) << "Session start. cmdline is [" << cmd << "]";

  // Setting the crash reporter.
  base::string16 pipe_name = ReplaceStringPlaceholders(kPipeNameFormat,
                                                 application_name,
                                                 NULL);
  cmd_line.AppendSwitch("no-window");
  cmd_line.AppendSwitchASCII("max-reports", "128");
  cmd_line.AppendSwitchASCII("reporter", ATOM_PROJECT_NAME "-crash-service");
  cmd_line.AppendSwitchNative("pipe-name", pipe_name);

  breakpad::CrashService crash_service;
  if (!crash_service.Initialize(application_name, operating_dir,
                                operating_dir))
    return 2;

  VLOG(1) << "Ready to process crash requests";

  // Enter the message loop.
  int retv = crash_service.ProcessingLoop();
  // Time to exit.
  VLOG(1) << "Session end. return code is " << retv;
  return retv;
}

}  // namespace crash_service
