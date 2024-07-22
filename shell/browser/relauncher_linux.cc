// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/relauncher.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>

#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"
#include "base/synchronization/waitable_event.h"

namespace relauncher::internal {

// this is global to be visible to the sa_handler
base::WaitableEvent parentWaiter;

void RelauncherSynchronizeWithParent() {
  base::ScopedFD relauncher_sync_fd(kRelauncherSyncFD);
  static const auto signum = SIGUSR2;

  // send signum to current process when parent process ends.
  if (HANDLE_EINTR(prctl(PR_SET_PDEATHSIG, signum)) != 0) {
    PLOG(ERROR) << "prctl";
    return;
  }

  // set up a signum handler
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = [](int /*signum*/) { parentWaiter.Signal(); };
  if (sigaction(signum, &action, nullptr) != 0) {
    PLOG(ERROR) << "sigaction";
    return;
  }

  // write a '\0' character to the pipe to the parent process.
  // this is how the parent knows that we're ready for it to exit.
  if (HANDLE_EINTR(write(relauncher_sync_fd.get(), "", 1)) != 1) {
    PLOG(ERROR) << "write";
    return;
  }

  // Wait for the parent to exit
  parentWaiter.Wait();
}

int LaunchProgram(const StringVector& relauncher_args,
                  const StringVector& argv) {
  // Redirect the stdout of child process to /dev/null, otherwise after
  // relaunch the child process will raise exception when writing to stdout.
  base::ScopedFD devnull(HANDLE_EINTR(open("/dev/null", O_WRONLY)));

  base::LaunchOptions options;
  options.allow_new_privs = true;
  options.new_process_group = true;  // detach
  options.fds_to_remap.emplace_back(devnull.get(), STDERR_FILENO);
  options.fds_to_remap.emplace_back(devnull.get(), STDOUT_FILENO);

  base::Process process = base::LaunchProcess(argv, options);
  return process.IsValid() ? 0 : 1;
}

}  // namespace relauncher::internal
