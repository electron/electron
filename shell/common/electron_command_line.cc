// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/electron_command_line.h"

#include "base/command_line.h"
#include "base/containers/to_vector.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"

namespace electron {

// static
base::CommandLine::StringVector& ElectronCommandLine::argv() {
  static base::NoDestructor<base::CommandLine::StringVector> g_argv;
  return *g_argv;
}

// static
void ElectronCommandLine::Init(int argc,
                               base::CommandLine::CharType const* const* argv) {
  DCHECK(ElectronCommandLine::argv().empty());

  // Safety: as is normal in command lines, argc and argv must correspond
  // to one another. Otherwise there will be out-of-bounds accesses.
  ElectronCommandLine::argv().assign(argv, UNSAFE_BUFFERS(argv + argc));
}

// static
std::vector<std::string> ElectronCommandLine::AsUtf8() {
  DCHECK(!argv().empty());

#if BUILDFLAG(IS_WIN)
  return base::ToVector(
      argv(), [](const auto& wstr) { return base::WideToUTF8(wstr); });
#else
  return argv();
#endif
}

#if BUILDFLAG(IS_LINUX)
// static
void ElectronCommandLine::InitializeFromCommandLine() {
  argv() = base::CommandLine::ForCurrentProcess()->argv();
}
#endif

}  // namespace electron
