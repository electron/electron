// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/electron_command_line.h"

#include "base/command_line.h"

namespace electron {

// static
base::CommandLine::StringVector ElectronCommandLine::argv_;

// static
void ElectronCommandLine::Init(int argc, base::CommandLine::CharType** argv) {
  DCHECK(argv_.empty());
  argv_.assign(argv, argv + argc);
}

#if BUILDFLAG(IS_LINUX)
// static
void ElectronCommandLine::InitializeFromCommandLine() {
  argv_ = base::CommandLine::ForCurrentProcess()->argv();
}
#endif

}  // namespace electron
