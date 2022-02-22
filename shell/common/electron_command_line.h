// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_ELECTRON_COMMAND_LINE_H_
#define ELECTRON_SHELL_COMMON_ELECTRON_COMMAND_LINE_H_

#include "base/command_line.h"
#include "build/build_config.h"

namespace electron {

// Singleton to remember the original "argc" and "argv".
class ElectronCommandLine {
 public:
  // disable copy
  ElectronCommandLine() = delete;
  ElectronCommandLine(const ElectronCommandLine&) = delete;
  ElectronCommandLine& operator=(const ElectronCommandLine&) = delete;

  static const base::CommandLine::StringVector& argv() { return argv_; }

  static void Init(int argc, base::CommandLine::CharType** argv);

#if BUILDFLAG(IS_LINUX)
  // On Linux the command line has to be read from base::CommandLine since
  // it is using zygote.
  static void InitializeFromCommandLine();
#endif

 private:
  static base::CommandLine::StringVector argv_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_ELECTRON_COMMAND_LINE_H_
