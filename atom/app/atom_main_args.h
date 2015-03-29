// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_ATOM_MAIN_ARGS_H_
#define ATOM_APP_ATOM_MAIN_ARGS_H_

#include <string>
#include <vector>

#include "base/logging.h"

namespace atom {

class AtomCommandLine {
 public:
  static void Init(int argc, const char* const* argv);
  static std::vector<std::string> argv() { return argv_; }

 private:
  static std::vector<std::string> argv_;

  DISALLOW_COPY_AND_ASSIGN(AtomCommandLine);
};

}  // namespace atom

#endif  // ATOM_APP_ATOM_MAIN_ARGS_H_
