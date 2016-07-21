// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/relauncher.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>

#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"

namespace relauncher {

namespace internal {

void RelauncherSynchronizeWithParent() {
  base::ScopedFD relauncher_sync_fd(kRelauncherSyncFD);

  // Don't execute signal handlers of SIGUSR2.
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR2);
  if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
    PLOG(ERROR) << "sigprocmask";
    return;
  }

  // Create a signalfd that watches for SIGUSR2.
  int usr2_fd = signalfd(-1, &mask, 0);
  if (usr2_fd < 0) {
    PLOG(ERROR) << "signalfd";
    return;
  }

  // Send SIGUSR2 to current process when parent process ends.
  if (HANDLE_EINTR(prctl(PR_SET_PDEATHSIG, SIGUSR2)) != 0) {
    PLOG(ERROR) << "prctl";
    return;
  }

  // Write a '\0' character to the pipe.
  if (HANDLE_EINTR(write(relauncher_sync_fd.get(), "", 1)) != 1) {
    PLOG(ERROR) << "write";
    return;
  }

  // Wait the SIGUSR2 signal to happen.
  struct signalfd_siginfo si;
  HANDLE_EINTR(read(usr2_fd, &si, sizeof(si)));
}

int LaunchProgram(const StringVector& relauncher_args,
                  const StringVector& argv) {
  // Redirect the stdout of child process to /dev/null, otherwise after
  // relaunch the child process will raise exception when writing to stdout.
  base::ScopedFD devnull(HANDLE_EINTR(open("/dev/null", O_WRONLY)));
  base::FileHandleMappingVector no_stdout;
  no_stdout.push_back(std::make_pair(devnull.get(), STDERR_FILENO));
  no_stdout.push_back(std::make_pair(devnull.get(), STDOUT_FILENO));

  base::LaunchOptions options;
  options.allow_new_privs = true;
  options.new_process_group = true;  // detach
  options.fds_to_remap = &no_stdout;
  base::Process process = base::LaunchProcess(argv, options);
  return process.IsValid() ? 0 : 1;
}

}  // namespace internal

}  // namespace relauncher
