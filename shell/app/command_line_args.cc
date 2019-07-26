// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/command_line_args.h"

namespace {

bool IsUrlArg(const base::CommandLine::CharType* arg) {
  // the first character must be a letter for this to be a URL
  auto c = *arg;
  if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
    for (auto* p = arg + 1; *p; ++p) {
      c = *p;

      // colon indicates that the argument starts with a URI scheme
      if (c == ':') {
        // it could also be a Windows filesystem path
        if (p == arg + 1)
          break;

        return true;
      }

      // white-space before a colon means it's not a URL
      if (c == ' ' || (0x9 <= c && c <= 0xD))
        break;
    }
  }

  return false;
}

}  // namespace

namespace electron {

bool CheckCommandLineArguments(int argc, base::CommandLine::CharType** argv) {
  const base::CommandLine::StringType dashdash(2, '-');
  bool block_args = false;
  for (int i = 0; i < argc; ++i) {
    if (argv[i] == dashdash)
      break;
    if (block_args) {
      return false;
    } else if (IsUrlArg(argv[i])) {
      block_args = true;
    }
  }
  return true;
}

}  // namespace electron
