// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// electron_wer.dll: a Windows Error Reporting (WER) runtime exception helper
// module. Windows loads this DLL out-of-process (inside WerFault.exe) for
// crashes that never reach crashpad's in-process unhandled-exception filter,
// most notably __fastfail() / STATUS_STACK_BUFFER_OVERRUN (security check
// failures, CFG violations, UCRT abort()), and asks the already-running
// crashpad handler to write a minidump for the crashed process. See
// //third_party/crashpad/crashpad/handler/win/wer/ and
// https://learn.microsoft.com/en-us/windows/win32/api/werapi/nf-werapi-werregisterruntimeexceptionmodule

#include <Windows.h>

// werapi.h must come after Windows.h.
#include <werapi.h>

#include "third_party/crashpad/crashpad/handler/win/wer/crashpad_wer.h"

extern "C" {

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
  return true;
}

// PFN_WER_RUNTIME_EXCEPTION_EVENT. |pContext| is the address of a
// crashpad::internal::WerRegistration in the target process.
HRESULT OutOfProcessExceptionEventCallback(
    PVOID pContext,
    const PWER_RUNTIME_EXCEPTION_INFORMATION pExceptionInformation,
    BOOL* pbOwnershipClaimed,
    PWSTR pwszEventName,
    PDWORD pchSize,
    PDWORD pdwSignatureCount) {
  // Exceptions that do not (reliably) reach crashpad's in-process handler.
  // Matches crashpad's own crashpad_wer module.
  static constexpr DWORD kWantedExceptions[] = {
      0xC0000005,  // STATUS_ACCESS_VIOLATION
      0xC000001D,  // STATUS_ILLEGAL_INSTRUCTION
      0xC0000409,  // STATUS_STACK_BUFFER_OVERRUN (__fastfail)
      0xC0000602,  // STATUS_FAIL_FAST_EXCEPTION
  };

  // Default to not claiming the event so other helpers / WER get a chance if
  // the dump could not be taken.
  *pbOwnershipClaimed = FALSE;

  const bool dumped = crashpad::wer::ExceptionEvent(
      kWantedExceptions,
      sizeof(kWantedExceptions) / sizeof(kWantedExceptions[0]), pContext,
      pExceptionInformation);
  if (dumped) {
    *pbOwnershipClaimed = TRUE;
    // The target was dumped and terminated; report failure so WER stops here.
    return E_FAIL;
  }
  return S_OK;
}

// PFN_WER_RUNTIME_EXCEPTION_EVENT_SIGNATURE. Never called, because ownership
// is only claimed after the process has been terminated.
HRESULT OutOfProcessExceptionEventSignatureCallback(
    PVOID pContext,
    const PWER_RUNTIME_EXCEPTION_INFORMATION pExceptionInformation,
    DWORD dwIndex,
    PWSTR pwszName,
    PDWORD pchName,
    PWSTR pwszValue,
    PDWORD pchValue) {
  return E_FAIL;
}

// PFN_WER_RUNTIME_EXCEPTION_DEBUGGER_LAUNCH. Never called, see above.
HRESULT OutOfProcessExceptionEventDebuggerLaunchCallback(
    PVOID pContext,
    const PWER_RUNTIME_EXCEPTION_INFORMATION pExceptionInformation,
    PBOOL pbIsCustomDebugger,
    PWSTR pwszDebuggerLaunch,
    PDWORD pchDebuggerLaunch,
    PBOOL pbIsDebuggerAutolaunch) {
  return E_FAIL;
}

}  // extern "C"
