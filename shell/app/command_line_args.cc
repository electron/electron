// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/command_line_args.h"

#include <locale>

#include "sandbox/policy/switches.h"
#include "shell/common/options_switches.h"

namespace {

bool IsUrlArg(const base::CommandLine::CharType* arg) {
  // the first character must be a letter for this to be a URL
  auto c = *arg;
  if (std::isalpha(c, std::locale::classic())) {
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
      if (std::isspace(c, std::locale::classic()))
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

bool IsSandboxEnabled(base::CommandLine* command_line) {
  return command_line->HasSwitch(switches::kEnableSandbox) ||
         !command_line->HasSwitch(sandbox::policy::switches::kNoSandbox);
}

}  // namespace electron
