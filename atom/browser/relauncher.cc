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
#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"

namespace relauncher {

namespace internal {

const int kRelauncherSyncFD = STDERR_FILENO + 1;
const char* kRelauncherTypeArg = "--type=relauncher";
const char* kRelauncherArgSeparator = "---";

}  // namespace internal

bool RelaunchApp(const std::vector<std::string>& args) {
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

  std::vector<std::string> relauncher_args;
  return RelaunchAppWithHelper(child_path.value(), relauncher_args, args);
}

bool RelaunchAppWithHelper(const std::string& helper,
                           const std::vector<std::string>& relauncher_args,
                           const std::vector<std::string>& args) {
  std::vector<std::string> relaunch_args;
  relaunch_args.push_back(helper);
  relaunch_args.push_back(internal::kRelauncherTypeArg);

  relaunch_args.insert(relaunch_args.end(),
                       relauncher_args.begin(), relauncher_args.end());

  relaunch_args.push_back(internal::kRelauncherArgSeparator);

  relaunch_args.insert(relaunch_args.end(), args.begin(), args.end());

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

  base::LaunchOptions options;
  options.fds_to_remap = &fd_map;
  if (!base::LaunchProcess(relaunch_args, options).IsValid()) {
    LOG(ERROR) << "base::LaunchProcess failed";
    return false;
  }

  // The relauncher process is now starting up, or has started up. The
  // original parent process continues.

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
  return true;
}

int RelauncherMain(const content::MainFunctionParams& main_parameters) {
  const std::vector<std::string>& argv = atom::AtomCommandLine::argv();

  if (argv.size() < 4 || argv[1] != internal::kRelauncherTypeArg) {
    LOG(ERROR) << "relauncher process invoked with unexpected arguments";
    return 1;
  }

  internal::RelauncherSynchronizeWithParent();

  // Figure out what to execute, what arguments to pass it, and whether to
  // start it in the background.
  bool in_relaunch_args = false;
  bool seen_relaunch_executable = false;
  std::string relaunch_executable;
  std::vector<std::string> relauncher_args;
  std::vector<std::string> launch_args;
  for (size_t argv_index = 2; argv_index < argv.size(); ++argv_index) {
    const std::string& arg(argv[argv_index]);
    if (!in_relaunch_args) {
      if (arg == internal::kRelauncherArgSeparator) {
        in_relaunch_args = true;
      } else {
        relauncher_args.push_back(arg);
      }
    } else {
      if (!seen_relaunch_executable) {
        // The first argument after kRelauncherBackgroundArg is the path to
        // the executable file or .app bundle directory. The Launch Services
        // interface wants this separate from the rest of the arguments. In
        // the relaunched process, this path will still be visible at argv[0].
        relaunch_executable.assign(arg);
        seen_relaunch_executable = true;
      } else {
        launch_args.push_back(arg);
      }
    }
  }

  if (!seen_relaunch_executable) {
    LOG(ERROR) << "nothing to relaunch";
    return 1;
  }

  if (internal::LaunchProgram(relauncher_args, relaunch_executable,
                              launch_args) != 0) {
    LOG(ERROR) << "failed to launch program";
    return 1;
  }

  // The application should have relaunched (or is in the process of
  // relaunching). From this point on, only clean-up tasks should occur, and
  // failures are tolerable.

  return 0;
}

}  // namespace relauncher
