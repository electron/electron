// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_ELECTRON_COMMAND_LINE_H_
#define SHELL_COMMON_ELECTRON_COMMAND_LINE_H_

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/macros.h"
#include "build/build_config.h"

namespace electron {

// Singleton to remember the original "argc" and "argv".
class ElectronCommandLine {
 public:
  static const base::CommandLine::StringVector& argv() { return argv_; }

  static void Init(int argc, base::CommandLine::CharType** argv);

#if defined(OS_LINUX)
  // On Linux the command line has to be read from base::CommandLine since
  // it is using zygote.
  static void InitializeFromCommandLine();
#endif

 private:
  static base::CommandLine::StringVector argv_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(ElectronCommandLine);
};

}  // namespace electron

#endif  // SHELL_COMMON_ELECTRON_COMMAND_LINE_H_
