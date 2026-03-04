// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/relauncher.h"

#include <windows.h>

#include "base/logging.h"
#include "base/process/launch.h"
#include "base/process/process_handle.h"
#include "base/strings/strcat_win.h"
#include "base/strings/string_number_conversions_win.h"
#include "base/win/scoped_handle.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/win_utils.h"
#include "shell/common/command_line_util_win.h"

namespace relauncher::internal {

namespace {

struct PROCESS_BASIC_INFORMATION {
  union {
    NTSTATUS ExitStatus;
    PVOID padding_for_x64_0;
  };
  PPEB PebBaseAddress;
  KAFFINITY AffinityMask;
  union {
    KPRIORITY BasePriority;
    PVOID padding_for_x64_1;
  };
  union {
    DWORD UniqueProcessId;
    PVOID padding_for_x64_2;
  };
  union {
    DWORD InheritedFromUniqueProcessId;
    PVOID padding_for_x64_3;
  };
};

HANDLE GetParentProcessHandle(base::ProcessHandle handle) {
  base::ProcessId ppid = base::GetParentProcessId(handle);
  if (ppid == 0u) {
    LOG(ERROR) << "Could not get parent process handle";
    return nullptr;
  }

  return ::OpenProcess(PROCESS_ALL_ACCESS, TRUE, ppid);
}

}  // namespace

StringType GetWaitEventName(base::ProcessId pid) {
  return base::StrCat({L"ElectronRelauncherWaitEvent-",
                       base::NumberToWString(static_cast<int>(pid))});
}

StringType ArgvToCommandLineString(const StringVector& argv) {
  StringType command_line;
  for (const StringType& arg : argv) {
    if (!command_line.empty())
      command_line += L' ';
    command_line += electron::AddQuoteForArg(arg);
  }
  return command_line;
}

void RelauncherSynchronizeWithParent() {
  base::Process process = base::Process::Current();
  base::win::ScopedHandle parent_process(
      GetParentProcessHandle(process.Handle()));

  // Notify the parent process that it can quit now.
  StringType name = internal::GetWaitEventName(process.Pid());
  base::win::ScopedHandle wait_event(
      CreateEvent(nullptr, TRUE, FALSE, name.c_str()));
  ::SetEvent(wait_event.Get());

  // Wait for parent process to quit.
  WaitForSingleObject(parent_process.Get(), INFINITE);
}

int LaunchProgram(const StringVector& relauncher_args,
                  const StringVector& argv) {
  base::LaunchOptions options;
  base::Process process =
      base::LaunchProcess(ArgvToCommandLineString(argv), options);
  return process.IsValid() ? 0 : 1;
}

}  // namespace relauncher::internal
