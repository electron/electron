// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_main_args.h"

namespace atom {

// static
std::vector<std::string> AtomCommandLine::argv_;

// static
void AtomCommandLine::Init(int argc, const char* const* argv) {
  for (int i = 0; i < argc; ++i) {
    argv_.push_back(argv[i]);
  }
}

}  // namespace atom
