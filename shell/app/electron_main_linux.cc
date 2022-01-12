// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <utility>

#include "base/at_exit.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "content/public/app/content_main.h"
#include "shell/app/electron_main_delegate.h"  // NOLINT
#include "shell/common/electron_command_line.h"

// Copied from //base/ignore_result.h, to avoid taking a dependency on //base
// on Mac.
template <typename T>
inline void ignore_result(const T&) {}

namespace {

ALLOW_UNUSED_TYPE bool IsEnvSet(const char* name) {
  char* indicator = getenv(name);
  return indicator && indicator[0] != '\0';
}

void FixStdioStreams() {
  // libuv may mark stdin/stdout/stderr as close-on-exec, which interferes
  // with chromium's subprocess spawning. As a workaround, we detect if these
  // streams are closed on startup, and reopen them as /dev/null if necessary.
  // Otherwise, an unrelated file descriptor will be assigned as stdout/stderr
  // which may cause various errors when attempting to write to them.
  //
  // For details see https://github.com/libuv/libuv/issues/2062
  struct stat st;
  if (fstat(STDIN_FILENO, &st) < 0 && errno == EBADF)
    ignore_result(freopen("/dev/null", "r", stdin));
  if (fstat(STDOUT_FILENO, &st) < 0 && errno == EBADF)
    ignore_result(freopen("/dev/null", "w", stdout));
  if (fstat(STDERR_FILENO, &st) < 0 && errno == EBADF)
    ignore_result(freopen("/dev/null", "w", stderr));
}

}  // namespace

int main(int argc, char* argv[]) {
  FixStdioStreams();

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  if (electron::fuses::IsRunAsNodeEnabled() && IsEnvSet(electron::kRunAsNode)) {
    base::i18n::InitializeICU();
    base::AtExitManager atexit_manager;
    return electron::NodeMain(argc, argv);
  }
#endif

  electron::ElectronMainDelegate delegate;
  content::ContentMainParams params(&delegate);
  electron::ElectronCommandLine::Init(argc, argv);
  params.argc = argc;
  params.argv = const_cast<const char**>(argv);
  base::CommandLine::Init(params.argc, params.argv);
  // TODO(https://crbug.com/1176772): Remove when Chrome Linux is fully migrated
  // to Crashpad.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      ::switches::kEnableCrashpad);
  return content::ContentMain(std::move(params));
}
