// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

namespace base {
class CommandLine;
class FilePath;
}  // namespace base

namespace electron {

namespace logging {

void InitLogging(const base::CommandLine& command_line);

base::FilePath GetLogFileName(const base::CommandLine& command_line);

}  // namespace logging

}  // namespace electron
