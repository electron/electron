// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/crash_reporter/win/crash_service_main.h"

#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/common/crash_reporter/crash_reporter.h"
#include "third_party/crashpad/crashpad/handler/handler_main.h"

namespace crash_service {

namespace {

const wchar_t kStandardLogFile[] = L"operation_log.txt";

void InvalidParameterHandler(const wchar_t*,
                             const wchar_t*,
                             const wchar_t*,
                             unsigned int,
                             uintptr_t) {
  // noop.
}

bool CreateCrashServiceDirectory(const base::FilePath& temp_dir) {
  if (!base::PathExists(temp_dir)) {
    if (!base::CreateDirectory(temp_dir))
      return false;
  }
  return true;
}

void RemoveArgs(std::vector<char*>* args) {
  args->erase(
      std::remove_if(args->begin(), args->end(), [](const std::string& str) {
        return base::StartsWith(str, "--type", base::CompareCase::SENSITIVE) ||
               base::StartsWith(
                   str,
                   std::string("--") + crash_reporter::kCrashesDirectoryKey,
                   base::CompareCase::INSENSITIVE_ASCII);
      }));
}

}  // namespace.

int Main(std::vector<char*>* args) {
  // Ignore invalid parameter errors.
  _set_invalid_parameter_handler(InvalidParameterHandler);

  // Initialize all Chromium things.
  base::AtExitManager exit_manager;
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  // We use/create a directory under the user's temp folder, for logging.
  base::FilePath operating_dir(
      cmd_line->GetSwitchValueNative(crash_reporter::kCrashesDirectoryKey));
  CreateCrashServiceDirectory(operating_dir);
  base::FilePath log_file_path = operating_dir.Append(kStandardLogFile);

  // Logging to stderr (to help with debugging failures on the
  // buildbots) and to a file.
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_ALL;
  settings.log_file_path = log_file_path.value().c_str();
  logging::InitLogging(settings);
  // Logging with pid, tid and timestamp.
  logging::SetLogItems(true, true, true, false);

  // Crashpad cannot handle unknown arguments, so we need to remove it
  RemoveArgs(args);
  return crashpad::HandlerMain(args->size(), args->data(), nullptr);
}

}  // namespace crash_service
