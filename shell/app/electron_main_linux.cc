// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/files/scoped_file.h"
#include "base/i18n/icu_util.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/cstring_view.h"
#include "content/public/app/content_main.h"
#include "electron/fuses.h"
#include "shell/app/electron_main_delegate.h"  // NOLINT
#include "shell/app/node_main.h"
#include "shell/app/uv_stdio_fix.h"
#include "shell/common/electron_command_line.h"
#include "shell/common/electron_constants.h"
#include "shell/common/uv_includes.h"

namespace {

[[nodiscard]] bool IsEnvSet(const base::cstring_view name) {
  const char* const indicator = getenv(name.c_str());
  return indicator && *indicator;
}

// Linux grows the descriptor table in powers of two and, once the process has
// threads, each expansion waits out an RCU grace period (tens of ms). Growing
// it to 1024 entries now is free; content's zygote does the same for the
// children it forks.
void PreallocateFileDescriptorTable() {
  base::ScopedFD dev_null(
      HANDLE_EINTR(open("/dev/null", O_RDONLY | O_CLOEXEC)));
  if (!dev_null.is_valid()) {
    return;
  }
  base::ScopedFD high_fd(
      HANDLE_EINTR(fcntl(dev_null.get(), F_DUPFD_CLOEXEC, 512)));
}

}  // namespace

int main(int argc, char* argv[]) {
  FixStdioStreams();

  // Chromium expects the original argv in its original memory location
  // to update /proc/<pid>/cmdline.
  const char** original_argv = UNSAFE_BUFFERS(const_cast<const char**>(argv));
  argv = uv_setup_args(argc, argv);
  base::CommandLine::Init(argc, argv);
  electron::ElectronCommandLine::Init(argc, argv);

  if (electron::fuses::IsRunAsNodeEnabled() && IsEnvSet(electron::kRunAsNode)) {
    PreallocateFileDescriptorTable();
    base::i18n::InitializeICU();
    base::AtExitManager atexit_manager;
    return electron::NodeMain();
  }

  electron::ElectronMainDelegate delegate;
  content::ContentMainParams params{&delegate};
  params.argc = argc;
  params.argv = original_argv;
  return content::ContentMain(std::move(params));
}
