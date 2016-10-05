// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/crash_reporter/crash_reporter_win.h"

#include <string>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/result_codes.h"
#include "gin/public/debug.h"
#include "sandbox/win/src/nt_internals.h"

#pragma intrinsic(_AddressOfReturnAddress)
#pragma intrinsic(_ReturnAddress)

#ifdef _WIN64
// See http://msdn.microsoft.com/en-us/library/ddssxxy8.aspx
typedef struct _UNWIND_INFO {
  unsigned char Version : 3;
  unsigned char Flags : 5;
  unsigned char SizeOfProlog;
  unsigned char CountOfCodes;
  unsigned char FrameRegister : 4;
  unsigned char FrameOffset : 4;
  ULONG ExceptionHandler;
} UNWIND_INFO, *PUNWIND_INFO;
#endif

namespace crash_reporter {

namespace {

// Minidump with stacks, PEB, TEB, and unloaded module list.
const MINIDUMP_TYPE kSmallDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithProcessThreadData |  // Get PEB and TEB.
    MiniDumpWithUnloadedModules);  // Get unloaded modules when available.

const wchar_t kWaitEventFormat[] = L"$1CrashServiceWaitEvent";
const wchar_t kPipeNameFormat[] = L"\\\\.\\pipe\\$1 Crash Service";

// Matches breakpad/src/client/windows/common/ipc_protocol.h.
const int kNameMaxLength = 64;
const int kValueMaxLength = 64;

typedef NTSTATUS (WINAPI* NtTerminateProcessPtr)(HANDLE ProcessHandle,
                                                 NTSTATUS ExitStatus);
char* g_real_terminate_process_stub = NULL;

void TerminateProcessWithoutDump() {
  // Patched stub exists based on conditions (See InitCrashReporter).
  // As a side note this function also gets called from
  // WindowProcExceptionFilter.
  if (g_real_terminate_process_stub == NULL) {
    ::TerminateProcess(::GetCurrentProcess(), content::RESULT_CODE_KILLED);
  } else {
    NtTerminateProcessPtr real_terminate_proc =
        reinterpret_cast<NtTerminateProcessPtr>(
            static_cast<char*>(g_real_terminate_process_stub));
    real_terminate_proc(::GetCurrentProcess(), content::RESULT_CODE_KILLED);
  }
}

#ifdef _WIN64
int CrashForExceptionInNonABICompliantCodeRange(
    PEXCEPTION_RECORD ExceptionRecord,
    ULONG64 EstablisherFrame,
    PCONTEXT ContextRecord,
    PDISPATCHER_CONTEXT DispatcherContext) {
  EXCEPTION_POINTERS info = { ExceptionRecord, ContextRecord };
  if (!CrashReporter::GetInstance())
    return EXCEPTION_CONTINUE_SEARCH;
  return static_cast<CrashReporterWin*>(CrashReporter::GetInstance())->
      CrashForException(&info);
}

struct ExceptionHandlerRecord {
  RUNTIME_FUNCTION runtime_function;
  UNWIND_INFO unwind_info;
  unsigned char thunk[12];
};

bool RegisterNonABICompliantCodeRange(void* start, size_t size_in_bytes) {
  ExceptionHandlerRecord* record =
      reinterpret_cast<ExceptionHandlerRecord*>(start);

  // We assume that the first page of the code range is executable and
  // committed and reserved for breakpad. What could possibly go wrong?

  // All addresses are 32bit relative offsets to start.
  record->runtime_function.BeginAddress = 0;
  record->runtime_function.EndAddress =
      base::checked_cast<DWORD>(size_in_bytes);
  record->runtime_function.UnwindData =
      offsetof(ExceptionHandlerRecord, unwind_info);

  // Create unwind info that only specifies an exception handler.
  record->unwind_info.Version = 1;
  record->unwind_info.Flags = UNW_FLAG_EHANDLER;
  record->unwind_info.SizeOfProlog = 0;
  record->unwind_info.CountOfCodes = 0;
  record->unwind_info.FrameRegister = 0;
  record->unwind_info.FrameOffset = 0;
  record->unwind_info.ExceptionHandler =
      offsetof(ExceptionHandlerRecord, thunk);

  // Hardcoded thunk.
  // mov imm64, rax
  record->thunk[0] = 0x48;
  record->thunk[1] = 0xb8;
  void* handler = &CrashForExceptionInNonABICompliantCodeRange;
  memcpy(&record->thunk[2], &handler, 8);

  // jmp rax
  record->thunk[10] = 0xff;
  record->thunk[11] = 0xe0;

  // Protect reserved page against modifications.
  DWORD old_protect;
  return VirtualProtect(start, sizeof(ExceptionHandlerRecord),
                        PAGE_EXECUTE_READ, &old_protect) &&
         RtlAddFunctionTable(&record->runtime_function, 1,
                             reinterpret_cast<DWORD64>(start));
}

void UnregisterNonABICompliantCodeRange(void* start) {
  ExceptionHandlerRecord* record =
      reinterpret_cast<ExceptionHandlerRecord*>(start);

  RtlDeleteFunctionTable(&record->runtime_function);
}
#endif  // _WIN64

}  // namespace

CrashReporterWin::CrashReporterWin()
    : skip_system_crash_handler_(false),
      code_range_registered_(false) {
}

CrashReporterWin::~CrashReporterWin() {
}

void CrashReporterWin::InitBreakpad(const std::string& product_name,
                                    const std::string& version,
                                    const std::string& company_name,
                                    const std::string& submit_url,
                                    bool auto_submit,
                                    bool skip_system_crash_handler) {
  skip_system_crash_handler_ = skip_system_crash_handler;

  base::FilePath temp_dir;
  if (!GetTempDirectory(&temp_dir)) {
    LOG(ERROR) << "Cannot get temp directory";
    return;
  }

  base::string16 pipe_name = base::ReplaceStringPlaceholders(
      kPipeNameFormat, base::UTF8ToUTF16(product_name), NULL);
  base::string16 wait_name = base::ReplaceStringPlaceholders(
      kWaitEventFormat, base::UTF8ToUTF16(product_name), NULL);

  // Wait until the crash service is started.
  HANDLE wait_event = ::CreateEventW(NULL, TRUE, FALSE, wait_name.c_str());
  if (wait_event != NULL) {
    WaitForSingleObject(wait_event, 1000);
    CloseHandle(wait_event);
  }

  // ExceptionHandler() attaches our handler and ~ExceptionHandler() detaches
  // it, so we must explicitly reset *before* we instantiate our new handler
  // to allow any previous handler to detach in the correct order.
  breakpad_.reset();

  breakpad_.reset(new google_breakpad::ExceptionHandler(
      temp_dir.value(),
      FilterCallback,
      MinidumpCallback,
      this,
      google_breakpad::ExceptionHandler::HANDLER_ALL,
      kSmallDumpType,
      pipe_name.c_str(),
      GetCustomInfo(product_name, version, company_name)));

  if (!breakpad_->IsOutOfProcess())
    LOG(ERROR) << "Cannot initialize out-of-process crash handler";

#ifdef _WIN64
  // Hook up V8 to breakpad.
  if (!code_range_registered_) {
    code_range_registered_ = true;
    // gin::Debug::SetCodeRangeCreatedCallback only runs the callback when
    // Isolate is just created, so we have to manually run following code here.
    void* code_range = nullptr;
    size_t size = 0;
    v8::Isolate::GetCurrent()->GetCodeRange(&code_range, &size);
    if (code_range && size &&
        RegisterNonABICompliantCodeRange(code_range, size)) {
      gin::Debug::SetCodeRangeDeletedCallback(
          UnregisterNonABICompliantCodeRange);
    }
  }
#endif
}

void CrashReporterWin::SetUploadParameters() {
  upload_parameters_["platform"] = "win32";
}

int CrashReporterWin::CrashForException(EXCEPTION_POINTERS* info) {
  if (breakpad_) {
    breakpad_->WriteMinidumpForException(info);
    TerminateProcessWithoutDump();
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

// static
bool CrashReporterWin::FilterCallback(void* context,
                                      EXCEPTION_POINTERS* exinfo,
                                      MDRawAssertionInfo* assertion) {
  return true;
}

// static
bool CrashReporterWin::MinidumpCallback(const wchar_t* dump_path,
                                        const wchar_t* minidump_id,
                                        void* context,
                                        EXCEPTION_POINTERS* exinfo,
                                        MDRawAssertionInfo* assertion,
                                        bool succeeded) {
  CrashReporterWin* self = static_cast<CrashReporterWin*>(context);
  if (succeeded && !self->skip_system_crash_handler_)
    return true;
  else
    return false;
}

google_breakpad::CustomClientInfo* CrashReporterWin::GetCustomInfo(
    const std::string& product_name,
    const std::string& version,
    const std::string& company_name) {
  custom_info_entries_.clear();
  custom_info_entries_.reserve(2 + upload_parameters_.size());

  custom_info_entries_.push_back(google_breakpad::CustomInfoEntry(
      L"prod", L"Electron"));
  custom_info_entries_.push_back(google_breakpad::CustomInfoEntry(
      L"ver", base::UTF8ToWide(version).c_str()));

  for (StringMap::const_iterator iter = upload_parameters_.begin();
       iter != upload_parameters_.end(); ++iter) {
    // breakpad has hardcoded the length of name/value, and doesn't truncate
    // the values itself, so we have to truncate them here otherwise weird
    // things may happen.
    std::wstring name = base::UTF8ToWide(iter->first);
    std::wstring value = base::UTF8ToWide(iter->second);
    if (name.length() > kNameMaxLength - 1)
      name.resize(kNameMaxLength - 1);
    if (value.length() > kValueMaxLength - 1)
      value.resize(kValueMaxLength - 1);

    custom_info_entries_.push_back(
        google_breakpad::CustomInfoEntry(name.c_str(), value.c_str()));
  }

  custom_info_.entries = &custom_info_entries_.front();
  custom_info_.count = custom_info_entries_.size();
  return &custom_info_;
}

// static
CrashReporterWin* CrashReporterWin::GetInstance() {
  return base::Singleton<CrashReporterWin>::get();
}

// static
CrashReporter* CrashReporter::GetInstance() {
  return CrashReporterWin::GetInstance();
}

}  // namespace crash_reporter
