// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_ATOM_COMMAND_LINE_H_
#define ATOM_COMMON_ATOM_COMMAND_LINE_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "build/build_config.h"

namespace atom {

// Singleton to remember the original "argc" and "argv".
class AtomCommandLine {
 public:
  static void Init(int argc, const char* const* argv);
  static std::vector<std::string> argv() { return argv_; }

#if defined(OS_WIN)
  static void InitW(int argc, const wchar_t* const* argv);
  static std::vector<std::wstring> wargv() { return wargv_; }
#endif

#if defined(OS_LINUX)
  // On Linux the command line has to be read from base::CommandLine since
  // it is using zygote.
  static void InitializeFromCommandLine();
#endif

 private:
  static std::vector<std::string> argv_;

#if defined(OS_WIN)
  static std::vector<std::wstring> wargv_;
#endif

  DISALLOW_IMPLICIT_CONSTRUCTORS(AtomCommandLine);
};

}  // namespace atom

#endif  // ATOM_COMMON_ATOM_COMMAND_LINE_H_
