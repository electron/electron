// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/atom_command_line.h"

#include "base/command_line.h"
#include "uv.h"  // NOLINT(build/include)

namespace atom {

// static
base::CommandLine::StringVector AtomCommandLine::argv_;

// static
void AtomCommandLine::Init(int argc, base::CommandLine::CharType** argv) {
  DCHECK(argv_.empty());

  // NOTE: uv_setup_args does nothing on Windows, so we don't need to call it.
  // Otherwise we'd have to convert the arguments from UTF16.
#if !defined(OS_WIN)
  // Hack around with the argv pointer. Used for process.title = "blah"
  argv = uv_setup_args(argc, argv);
#endif

  argv_.assign(argv, argv + argc);
}

#if defined(OS_LINUX)
// static
void AtomCommandLine::InitializeFromCommandLine() {
  argv_ = base::CommandLine::ForCurrentProcess()->argv();
}
#endif

}  // namespace atom
