// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_ATOM_MAIN_ARGS_H_
#define ATOM_APP_ATOM_MAIN_ARGS_H_

#include <string>
#include <vector>

#include "base/macros.h"

namespace atom {

// Singleton to remember the original "argc" and "argv".
class AtomCommandLine {
 public:
  static void Init(int argc, const char* const* argv);
  static std::vector<std::string> argv() { return argv_; }

 private:
  static std::vector<std::string> argv_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(AtomCommandLine);
};

}  // namespace atom

#endif  // ATOM_APP_ATOM_MAIN_ARGS_H_
