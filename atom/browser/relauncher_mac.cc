// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/relauncher.h"

#include <ApplicationServices/ApplicationServices.h>

#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "atom/common/atom_command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/mac/mac_logging.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/sys_string_conversions.h"

namespace relauncher {

namespace internal {

namespace {

// The beginning of the "process serial number" argument that Launch Services
// sometimes inserts into command lines. A process serial number is only valid
// for a single process, so any PSN arguments will be stripped from command
// lines during relaunch to avoid confusion.
const char kPSNArg[] = "-psn_";

}  // namespace

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
  if (kevent(kq.get(), &change, 1, NULL, 0, NULL) == -1) {
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
  int events = kevent(kq.get(), NULL, 0, &event, 1, NULL);
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

int RelauncherMain(const content::MainFunctionParams& main_parameters) {
  const std::vector<std::string>& argv = atom::AtomCommandLine::argv();

  if (argv.size() < 4 || kRelauncherTypeArg != argv[1]) {
    LOG(ERROR) << "relauncher process invoked with unexpected arguments";
    return 1;
  }

  internal::RelauncherSynchronizeWithParent();

  // The capacity for relaunch_args is 4 less than argc, because it
  // won't contain the argv[0] of the relauncher process, the
  // RelauncherTypeArg() at argv[1], kRelauncherArgSeparator, or the
  // executable path of the process to be launched.
  base::ScopedCFTypeRef<CFMutableArrayRef> relaunch_args(
      CFArrayCreateMutable(NULL, argv.size() - 4, &kCFTypeArrayCallBacks));
  if (!relaunch_args) {
    LOG(ERROR) << "CFArrayCreateMutable";
    return 1;
  }

  // Figure out what to execute, what arguments to pass it, and whether to
  // start it in the background.
  bool in_relaunch_args = false;
  bool seen_relaunch_executable = false;
  std::string relaunch_executable;
  const std::string relauncher_arg_separator(kRelauncherArgSeparator);
  for (size_t argv_index = 2; argv_index < argv.size(); ++argv_index) {
    const std::string& arg(argv[argv_index]);

    // Strip any -psn_ arguments, as they apply to a specific process.
    if (arg.compare(0, strlen(kPSNArg), kPSNArg) == 0) {
      continue;
    }

    if (!in_relaunch_args && arg == relauncher_arg_separator) {
      in_relaunch_args = true;
    } else {
      if (!seen_relaunch_executable) {
        // The first argument after kRelauncherBackgroundArg is the path to
        // the executable file or .app bundle directory. The Launch Services
        // interface wants this separate from the rest of the arguments. In
        // the relaunched process, this path will still be visible at argv[0].
        relaunch_executable.assign(arg);
        seen_relaunch_executable = true;
      } else {
        base::ScopedCFTypeRef<CFStringRef> arg_cf(
            base::SysUTF8ToCFStringRef(arg));
        if (!arg_cf) {
          LOG(ERROR) << "base::SysUTF8ToCFStringRef failed for " << arg;
          return 1;
        }
        CFArrayAppendValue(relaunch_args, arg_cf);
      }
    }
  }

  if (!seen_relaunch_executable) {
    LOG(ERROR) << "nothing to relaunch";
    return 1;
  }

  FSRef app_fsref;
  if (!base::mac::FSRefFromPath(relaunch_executable, &app_fsref)) {
    LOG(ERROR) << "base::mac::FSRefFromPath failed for " << relaunch_executable;
    return 1;
  }

  LSApplicationParameters ls_parameters = {
    0,  // version
    kLSLaunchDefaults | kLSLaunchAndDisplayErrors | kLSLaunchNewInstance,
    &app_fsref,
    NULL,  // asyncLaunchRefCon
    NULL,  // environment
    relaunch_args,
    NULL   // initialEvent
  };

  OSStatus status = LSOpenApplication(&ls_parameters, NULL);
  if (status != noErr) {
    OSSTATUS_LOG(ERROR, status) << "LSOpenApplication";
    return 1;
  }

  // The application should have relaunched (or is in the process of
  // relaunching). From this point on, only clean-up tasks should occur, and
  // failures are tolerable.

  return 0;
}

}  // namespace internal

}  // namespace relauncher
