// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/command_line_args.h"

#include <algorithm>
#include <locale>

#include "sandbox/policy/switches.h"
#include "shell/common/options_switches.h"

namespace {

#if BUILDFLAG(IS_WIN)
constexpr auto DashDash = base::CommandLine::StringViewType{L"--"};
#else
constexpr auto DashDash = base::CommandLine::StringViewType{"--"};
#endif

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

// Check for CVE-2018-1000006 issues. Return true iff argv looks safe.
// Sample exploit: 'exodus://aaaaaaaaa" --gpu-launcher="cmd" --aaaaa='
// Prevent it by returning false if any arg except '--' follows a URL arg.
// More info at https://www.electronjs.org/blog/protocol-handler-fix
bool CheckCommandLineArguments(const base::CommandLine::StringVector& argv) {
  bool block_args = false;
  for (const auto& arg : argv) {
    if (arg == DashDash)
      break;
    if (block_args)
      return false;
    if (IsUrlArg(arg))
      block_args = true;
  }
  return true;
}

bool IsSandboxEnabled(base::CommandLine* command_line) {
  return command_line->HasSwitch(switches::kEnableSandbox) ||
         !command_line->HasSwitch(sandbox::policy::switches::kNoSandbox);
}

}  // namespace electron
