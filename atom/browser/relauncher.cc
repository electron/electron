// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/relauncher.h"

#include <string>
#include <vector>

#include "atom/common/atom_command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/strings/string_util.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"

#if defined(OS_POSIX)
#include "base/posix/eintr_wrapper.h"
#endif

namespace relauncher {

namespace internal {

#if defined(OS_POSIX)
const int kRelauncherSyncFD = STDERR_FILENO + 1;
#endif

const CharType* kRelauncherTypeArg = FILE_PATH_LITERAL("--type=relauncher");
const CharType* kRelauncherArgSeparator = FILE_PATH_LITERAL("---");

}  // namespace internal

bool RelaunchApp(const base::FilePath& app, const StringVector& args) {
  // Use the currently-running application's helper process. The automatic
  // update feature is careful to leave the currently-running version alone,
  // so this is safe even if the relaunch is the result of an update having
  // been applied. In fact, it's safer than using the updated version of the
  // helper process, because there's no guarantee that the updated version's
  // relauncher implementation will be compatible with the running version's.
  base::FilePath child_path;
  if (!PathService::Get(content::CHILD_PROCESS_EXE, &child_path)) {
    LOG(ERROR) << "No CHILD_PROCESS_EXE";
    return false;
  }

  StringVector relauncher_args;
  return RelaunchAppWithHelper(app, child_path, relauncher_args, args);
}

bool RelaunchAppWithHelper(const base::FilePath& app,
                           const base::FilePath& helper,
                           const StringVector& relauncher_args,
                           const StringVector& args) {
  StringVector relaunch_args;
  relaunch_args.push_back(helper.value());
  relaunch_args.push_back(internal::kRelauncherTypeArg);

  relaunch_args.insert(relaunch_args.end(),
                       relauncher_args.begin(), relauncher_args.end());

  relaunch_args.push_back(internal::kRelauncherArgSeparator);

  relaunch_args.push_back(app.value());
  relaunch_args.insert(relaunch_args.end(), args.begin(), args.end());

#if defined(OS_POSIX)
  int pipe_fds[2];
  if (HANDLE_EINTR(pipe(pipe_fds)) != 0) {
    PLOG(ERROR) << "pipe";
    return false;
  }

  // The parent process will only use pipe_read_fd as the read side of the
  // pipe. It can close the write side as soon as the relauncher process has
  // forked off. The relauncher process will only use pipe_write_fd as the
  // write side of the pipe. In that process, the read side will be closed by
  // base::LaunchApp because it won't be present in fd_map, and the write side
  // will be remapped to kRelauncherSyncFD by fd_map.
  base::ScopedFD pipe_read_fd(pipe_fds[0]);
  base::ScopedFD pipe_write_fd(pipe_fds[1]);

  // Make sure kRelauncherSyncFD is a safe value. base::LaunchProcess will
  // preserve these three FDs in forked processes, so kRelauncherSyncFD should
  // not conflict with them.
  static_assert(internal::kRelauncherSyncFD != STDIN_FILENO &&
                internal::kRelauncherSyncFD != STDOUT_FILENO &&
                internal::kRelauncherSyncFD != STDERR_FILENO,
                "kRelauncherSyncFD must not conflict with stdio fds");

  base::FileHandleMappingVector fd_map;
  fd_map.push_back(
      std::make_pair(pipe_write_fd.get(), internal::kRelauncherSyncFD));
#endif

  base::LaunchOptions options;
#if defined(OS_POSIX)
  options.fds_to_remap = &fd_map;
  base::Process process = base::LaunchProcess(relaunch_args, options);
#elif defined(OS_WIN)
  base::Process process = base::LaunchProcess(
      base::JoinString(relaunch_args, L" "), options);
#endif
  if (!process.IsValid()) {
    LOG(ERROR) << "base::LaunchProcess failed";
    return false;
  }

  // The relauncher process is now starting up, or has started up. The
  // original parent process continues.

#if defined(OS_WIN)
  // Synchronize with the relauncher process.
  StringType name = internal::GetWaitEventName(process.Pid());
  HANDLE wait_event = ::CreateEventW(NULL, TRUE, FALSE, name.c_str());
  if (wait_event != NULL) {
    WaitForSingleObject(wait_event, 1000);
    CloseHandle(wait_event);
  }
#elif defined(OS_POSIX)
  pipe_write_fd.reset();  // close(pipe_fds[1]);

  // Synchronize with the relauncher process.
  char read_char;
  int read_result = HANDLE_EINTR(read(pipe_read_fd.get(), &read_char, 1));
  if (read_result != 1) {
    if (read_result < 0) {
      PLOG(ERROR) << "read";
    } else {
      LOG(ERROR) << "read: unexpected result " << read_result;
    }
    return false;
  }

  // Since a byte has been successfully read from the relauncher process, it's
  // guaranteed to have set up its kqueue monitoring this process for exit.
  // It's safe to exit now.
#endif
  return true;
}

int RelauncherMain(const content::MainFunctionParams& main_parameters) {
#if defined(OS_WIN)
  const StringVector& argv = atom::AtomCommandLine::wargv();
#else
  const StringVector& argv = atom::AtomCommandLine::argv();
#endif

  if (argv.size() < 4 || argv[1] != internal::kRelauncherTypeArg) {
    LOG(ERROR) << "relauncher process invoked with unexpected arguments";
    return 1;
  }

  internal::RelauncherSynchronizeWithParent();

  // Figure out what to execute, what arguments to pass it, and whether to
  // start it in the background.
  bool in_relaunch_args = false;
  StringType relaunch_executable;
  StringVector relauncher_args;
  StringVector launch_argv;
  for (size_t argv_index = 2; argv_index < argv.size(); ++argv_index) {
    const StringType& arg(argv[argv_index]);
    if (!in_relaunch_args) {
      if (arg == internal::kRelauncherArgSeparator) {
        in_relaunch_args = true;
      } else {
        relauncher_args.push_back(arg);
      }
    } else {
      launch_argv.push_back(arg);
    }
  }

  if (launch_argv.empty()) {
    LOG(ERROR) << "nothing to relaunch";
    return 1;
  }

  if (internal::LaunchProgram(relauncher_args, launch_argv) != 0) {
    LOG(ERROR) << "failed to launch program";
    return 1;
  }

  // The application should have relaunched (or is in the process of
  // relaunching). From this point on, only clean-up tasks should occur, and
  // failures are tolerable.

  return 0;
}

}  // namespace relauncher
