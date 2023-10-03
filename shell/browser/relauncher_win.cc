// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/relauncher.h"

#include <windows.h>

#include "base/logging.h"
#include "base/process/launch.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_handle.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/win_utils.h"
#include "ui/base/win/shell.h"

namespace relauncher::internal {

namespace {

const CharType* kWaitEventName = L"ElectronRelauncherWaitEvent";

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
  NtQueryInformationProcessFunction NtQueryInformationProcess = nullptr;
  ResolveNTFunctionPtr("NtQueryInformationProcess", &NtQueryInformationProcess);
  if (!NtQueryInformationProcess) {
    LOG(ERROR) << "Unable to get NtQueryInformationProcess";
    return nullptr;
  }

  PROCESS_BASIC_INFORMATION pbi;
  LONG status =
      NtQueryInformationProcess(handle, ProcessBasicInformation, &pbi,
                                sizeof(PROCESS_BASIC_INFORMATION), nullptr);
  if (!NT_SUCCESS(status)) {
    LOG(ERROR) << "NtQueryInformationProcess failed";
    return nullptr;
  }

  return ::OpenProcess(PROCESS_ALL_ACCESS, TRUE,
                       pbi.InheritedFromUniqueProcessId);
}

StringType AddQuoteForArg(const StringType& arg) {
  // We follow the quoting rules of CommandLineToArgvW.
  // http://msdn.microsoft.com/en-us/library/17w5ykft.aspx
  std::wstring quotable_chars(L" \\\"");
  if (arg.find_first_of(quotable_chars) == std::wstring::npos) {
    // No quoting necessary.
    return arg;
  }

  std::wstring out;
  out.push_back(L'"');
  for (size_t i = 0; i < arg.size(); ++i) {
    if (arg[i] == '\\') {
      // Find the extent of this run of backslashes.
      size_t start = i, end = start + 1;
      for (; end < arg.size() && arg[end] == '\\'; ++end) {
      }
      size_t backslash_count = end - start;

      // Backslashes are escapes only if the run is followed by a double quote.
      // Since we also will end the string with a double quote, we escape for
      // either a double quote or the end of the string.
      if (end == arg.size() || arg[end] == '"') {
        // To quote, we need to output 2x as many backslashes.
        backslash_count *= 2;
      }
      for (size_t j = 0; j < backslash_count; ++j)
        out.push_back('\\');

      // Advance i to one before the end to balance i++ in loop.
      i = end - 1;
    } else if (arg[i] == '"') {
      out.push_back('\\');
      out.push_back('"');
    } else {
      out.push_back(arg[i]);
    }
  }
  out.push_back('"');

  return out;
}

}  // namespace

StringType GetWaitEventName(base::ProcessId pid) {
  return base::UTF8ToWide(
      base::StringPrintf("%ls-%d", kWaitEventName, static_cast<int>(pid)));
}

StringType ArgvToCommandLineString(const StringVector& argv) {
  StringType command_line;
  for (const StringType& arg : argv) {
    if (!command_line.empty())
      command_line += L' ';
    command_line += AddQuoteForArg(arg);
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
