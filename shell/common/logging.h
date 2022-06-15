// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_LOGGING_H_
#define ELECTRON_SHELL_COMMON_LOGGING_H_

namespace base {
class CommandLine;
class FilePath;
}  // namespace base

namespace logging {

void InitElectronLogging(const base::CommandLine& command_line,
                         bool is_preinit);

base::FilePath GetLogFileName(const base::CommandLine& command_line);

}  // namespace logging

#endif  // ELECTRON_SHELL_COMMON_LOGGING_H_
