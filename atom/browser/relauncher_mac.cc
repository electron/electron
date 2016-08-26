// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/relauncher.h"

#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/mac/mac_logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"
#include "base/strings/sys_string_conversions.h"

namespace relauncher {

namespace internal {

void RelauncherSynchronizeWithParent() {
  base::ScopedFD relauncher_sync_fd(kRelauncherSyncFD);

  int parent_pid = getppid();

  // PID 1 identifies init. launchd, that is. launchd never starts the
  // relauncher process directly, having this parent_pid means that the parent
  // already exited and launchd "inherited" the relauncher as its child.
  // There's no reason to synchronize with launchd.
  if (parent_pid == 1) {
    LOG(ERROR) << "unexpected parent_pid";
    return;
  }

  // Set up a kqueue to monitor the parent process for exit.
  base::ScopedFD kq(kqueue());
  if (!kq.is_valid()) {
    PLOG(ERROR) << "kqueue";
    return;
  }

  struct kevent change = { 0 };
  EV_SET(&change, parent_pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, NULL);
  if (kevent(kq.get(), &change, 1, nullptr, 0, nullptr) == -1) {
    PLOG(ERROR) << "kevent (add)";
    return;
  }

  // Write a '\0' character to the pipe.
  if (HANDLE_EINTR(write(relauncher_sync_fd.get(), "", 1)) != 1) {
    PLOG(ERROR) << "write";
    return;
  }

  // Up until now, the parent process was blocked in a read waiting for the
  // write above to complete. The parent process is now free to exit. Wait for
  // that to happen.
  struct kevent event;
  int events = kevent(kq.get(), nullptr, 0, &event, 1, nullptr);
  if (events != 1) {
    if (events < 0) {
      PLOG(ERROR) << "kevent (monitor)";
    } else {
      LOG(ERROR) << "kevent (monitor): unexpected result " << events;
    }
    return;
  }

  if (event.filter != EVFILT_PROC ||
      event.fflags != NOTE_EXIT ||
      event.ident != static_cast<uintptr_t>(parent_pid)) {
    LOG(ERROR) << "kevent (monitor): unexpected event, filter " << event.filter
               << ", fflags " << event.fflags << ", ident " << event.ident;
    return;
  }
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
  options.new_process_group = true;  // detach
  options.fds_to_remap = &no_stdout;
  base::Process process = base::LaunchProcess(argv, options);
  return process.IsValid() ? 0 : 1;
}

}  // namespace internal

}  // namespace relauncher
