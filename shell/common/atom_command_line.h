// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_ATOM_COMMAND_LINE_H_
#define ATOM_COMMON_ATOM_COMMAND_LINE_H_

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/macros.h"
#include "build/build_config.h"

namespace atom {

// Singleton to remember the original "argc" and "argv".
class AtomCommandLine {
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

  DISALLOW_IMPLICIT_CONSTRUCTORS(AtomCommandLine);
};

}  // namespace atom

#endif  // ATOM_COMMON_ATOM_COMMAND_LINE_H_
