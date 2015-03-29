// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_ATOM_MAIN_ARGS_H_
#define ATOM_APP_ATOM_MAIN_ARGS_H_

#include <vector>

#include "base/logging.h"

namespace atom {

// Singleton to remember the original "argc" and "argv".
class AtomCommandLine {
 public:
  static void Init(int argc, const char* argv[]);
  static std::vector<const char*> argv() { return argv_; }

 private:
  static std::vector<const char*> argv_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(AtomCommandLine);
};

}  // namespace atom

#endif  // ATOM_APP_ATOM_MAIN_ARGS_H_
