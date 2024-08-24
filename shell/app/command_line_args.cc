// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/command_line_args.h"

#include <algorithm>
#include <locale>

#include "sandbox/policy/switches.h"
#include "shell/common/options_switches.h"

namespace {

// we say it's a URL arg if it starts with a URI scheme that:
// 1. starts with an alpha, and
// 2. contains no spaces, and
// 3. is longer than one char (to ensure it's not a Windows drive path)
bool IsUrlArg(const base::CommandLine::StringViewType arg) {
  const auto scheme_end = arg.find(':');
  if (scheme_end == base::CommandLine::StringViewType::npos)
    return false;

  const auto& c_locale = std::locale::classic();
  const auto isspace = [&](auto ch) { return std::isspace(ch, c_locale); };
  const auto scheme = arg.substr(0U, scheme_end);
  return std::size(scheme) > 1U && std::isalpha(scheme.front(), c_locale) &&
         std::ranges::none_of(scheme, isspace);
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
