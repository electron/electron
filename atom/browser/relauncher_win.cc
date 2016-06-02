// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/relauncher.h"

#include <windows.h>

#include "base/process/launch.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/win/scoped_handle.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/win_utils.h"
#include "ui/base/win/shell.h"

namespace relauncher {

namespace internal {

namespace {

const CharType* kWaitEventName = L"ElectronRelauncherWaitEvent";

HANDLE GetParentProcessHandle(base::ProcessHandle handle) {
  NtQueryInformationProcessFunction NtQueryInformationProcess = nullptr;
  ResolveNTFunctionPtr("NtQueryInformationProcess", &NtQueryInformationProcess);
  if (!NtQueryInformationProcess) {
    LOG(ERROR) << "Unable to get NtQueryInformationProcess";
    return NULL;
  }

  PROCESS_BASIC_INFORMATION pbi;
	LONG status = NtQueryInformationProcess(
      handle, ProcessBasicInformation,
      &pbi, sizeof(PROCESS_BASIC_INFORMATION), NULL);
  if (!NT_SUCCESS(status)) {
    LOG(ERROR) << "NtQueryInformationProcess failed";
    return NULL;
  }

  return ::OpenProcess(PROCESS_ALL_ACCESS, TRUE,
                       pbi.InheritedFromUniqueProcessId);
}

}  // namespace

StringType GetWaitEventName(base::ProcessId pid) {
  return base::StringPrintf(L"%s-%d", kWaitEventName, static_cast<int>(pid));
}

void RelauncherSynchronizeWithParent() {
  base::Process process = base::Process::Current();
  base::win::ScopedHandle parent_process(
      GetParentProcessHandle(process.Handle()));

  // Notify the parent process that it can quit now.
  StringType name = internal::GetWaitEventName(process.Pid());
  base::win::ScopedHandle wait_event(
      ::CreateEventW(NULL, TRUE, FALSE, name.c_str()));
  ::SetEvent(wait_event.Get());

  // Wait for parent process to quit.
  WaitForSingleObject(parent_process.Get(), INFINITE);
}

int LaunchProgram(const StringVector& relauncher_args,
                  const StringType& program,
                  const StringVector& args) {
  base::LaunchOptions options;
  base::Process process = base::LaunchProcess(
      program + L" " + base::JoinString(args, L" "), options);
  return process.IsValid() ? 0 : 1;
}

}  // namespace internal

}  // namespace relauncher
