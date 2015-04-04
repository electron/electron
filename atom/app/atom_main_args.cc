// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_main_args.h"
#include "vendor/node/deps/uv/include/uv.h"

namespace atom {

// static
std::vector<std::string> AtomCommandLine::argv_;

// static
void AtomCommandLine::Init(int argc, const char* const* argv) {
  // Hack around with the argv pointer. Used for process.title = "blah"
  char** new_argv = uv_setup_args(argc, const_cast<char**>(argv));
  for (int i = 0; i < argc; ++i) {
    argv_.push_back(new_argv[i]);
  }
}

}  // namespace atom
