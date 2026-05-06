// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/command_line_args.h"

#include <algorithm>
#include <locale>

#include "sandbox/policy/switches.h"
#include "shell/common/options_switches.h"

namespace electron::internal {

namespace {

#if BUILDFLAG(IS_WIN)
constexpr auto DashDash = base::CommandLine::StringViewType{L"--"};
#else
constexpr auto DashDash = base::CommandLine::StringViewType{"--"};
#endif

}  // namespace

// We classify an arg as a URL iff its leading `<scheme>:` matches a valid
// RFC 3986 §3.1 URI scheme:
//
//     scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
//
// Plus two Electron-specific extensions:
//   1. The scheme must be longer than one character so we don't mistake
//      Windows drive-letter paths (`C:\...`) for URLs.
//   2. If a candidate scheme contains '.' and the suffix is a numeric
//      TCP port (1-65535), classify as a `host:port` positional argument
//      rather than a URL. This disambiguates SSH/scp-style invocations
//      like `host.example.com:22 --identity FOO` from dotted URI schemes
//      like `iris.beep://server` or `view-source:https://x.com`.
//
// The previous implementation accepted any non-whitespace characters in
// the scheme position, which mis-classified common SSH-style positional
// arguments (`user@host:22`) and Windows path-style values (`key=C:\Path`)
// as URLs. On Windows that caused `app user@host:port --some-flag value`
// to abort with exit code -1 in `wWinMain` because `CheckCommandLineArguments`
// blocks any positional arg following a URL arg. The combination of
// (a) RFC 3986 scheme conformance and (b) host:port disambiguation
// preserves the CVE-2018-1000006 mitigation for every IANA-registered URL
// scheme (including dotted ones such as `iris.beep`,
// `microsoft.windows.camera`, and `z39.50` whose payloads are hierarchical or
// non-numeric) while letting SSH-style positionals through.
bool IsUrlArg(const base::CommandLine::StringViewType arg) {
  const auto scheme_end = arg.find(':');
  if (scheme_end == base::CommandLine::StringViewType::npos)
    return false;
  const auto scheme = arg.substr(0U, scheme_end);
  if (std::size(scheme) <= 1U)
    return false;
  const auto& c_locale = std::locale::classic();
  if (!std::isalpha(scheme.front(), c_locale))
    return false;
  if (!std::all_of(scheme.begin() + 1, scheme.end(), [&c_locale](auto ch) {
        return std::isalnum(ch, c_locale) || ch == '+' || ch == '-' ||
               ch == '.';
      }))
    return false;
  // Disambiguate `host.example.com:port` from dotted URI schemes.
  if (scheme.find('.') != base::CommandLine::StringViewType::npos) {
    const auto suffix = arg.substr(scheme_end + 1U);
    if (!suffix.empty() && std::size(suffix) <= 5U &&
        std::all_of(suffix.begin(), suffix.end(), [&c_locale](auto ch) {
          return std::isdigit(ch, c_locale);
        })) {
      unsigned port = 0;
      for (auto ch : suffix)
        port = port * 10U + static_cast<unsigned>(ch - '0');
      if (port >= 1U && port <= 65535U)
        return false;
    }
  }
  return true;
}

}  // namespace electron::internal

namespace electron {

// Check for CVE-2018-1000006 issues. Return true iff argv looks safe.
// Sample exploit: 'exodus://aaaaaaaaa" --gpu-launcher="cmd" --aaaaa='
// Prevent it by returning false if any arg except '--' follows a URL arg.
// More info at https://www.electronjs.org/blog/protocol-handler-fix
bool CheckCommandLineArguments(const base::CommandLine::StringVector& argv) {
  bool block_args = false;
  for (const auto& arg : argv) {
    if (arg == internal::DashDash)
      break;
    if (block_args)
      return false;
    if (internal::IsUrlArg(arg))
      block_args = true;
  }
  return true;
}

bool IsSandboxEnabled(base::CommandLine* command_line) {
  return command_line->HasSwitch(switches::kEnableSandbox) ||
         !command_line->HasSwitch(sandbox::policy::switches::kNoSandbox);
}

}  // namespace electron
