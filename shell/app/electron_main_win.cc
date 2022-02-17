// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <windows.h>  // windows.h must be included first

#include <atlbase.h>  // ensures that ATL statics like `_AtlWinModule` are initialized (it's an issue in static debug build)
#include <shellapi.h>
#include <shellscalingapi.h>
#include <tchar.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/at_exit.h"
#include "base/environment.h"
#include "base/i18n/icu_util.h"
#include "base/process/launch.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "components/browser_watcher/exit_code_watcher_win.h"
#include "components/crash/core/app/crash_switches.h"
#include "components/crash/core/app/run_as_crashpad_handler_win.h"
#include "content/public/app/content_main.h"
#include "content/public/app/sandbox_helper_win.h"
#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "sandbox/win/src/sandbox_types.h"
#include "shell/app/command_line_args.h"
#include "shell/app/electron_main_delegate.h"
#include "shell/app/node_main.h"
#include "shell/common/electron_command_line.h"
#include "shell/common/electron_constants.h"
#include "third_party/crashpad/crashpad/util/win/initial_client_data.h"

namespace {

// Redefined here so we don't have to introduce a dependency on //content
// from //electron:electron_app
const char kUserDataDir[] = "user-data-dir";
const char kProcessType[] = "type";

bool IsEnvSet(const char* name) {
  size_t required_size;
  getenv_s(&required_size, nullptr, 0, name);
  return required_size != 0;
}

}  // namespace

namespace crash_reporter {
extern const char kCrashpadProcess[];
}

// In 32-bit builds, the main thread starts with the default (small) stack size.
// The ARCH_CPU_32_BITS blocks here and below are in support of moving the main
// thread to a fiber with a larger stack size.
#if defined(ARCH_CPU_32_BITS)
// The information needed to transfer control to the large-stack fiber and later
// pass the main routine's exit code back to the small-stack fiber prior to
// termination.
struct FiberState {
  HINSTANCE instance;
  LPVOID original_fiber;
  int fiber_result;
};

// A PFIBER_START_ROUTINE function run on a large-stack fiber that calls the
// main routine, stores its return value, and returns control to the small-stack
// fiber. |params| must be a pointer to a FiberState struct.
void WINAPI FiberBinder(void* params) {
  auto* fiber_state = static_cast<FiberState*>(params);
  // Call the wWinMain routine from the fiber. Reusing the entry point minimizes
  // confusion when examining call stacks in crash reports - seeing wWinMain on
  // the stack is a handy hint that this is the main thread of the process.
  fiber_state->fiber_result =
      wWinMain(fiber_state->instance, nullptr, nullptr, 0);
  // Switch back to the main thread to exit.
  ::SwitchToFiber(fiber_state->original_fiber);
}
#endif  // defined(ARCH_CPU_32_BITS)

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmd, int) {
#if defined(ARCH_CPU_32_BITS)
  enum class FiberStatus { kConvertFailed, kCreateFiberFailed, kSuccess };
  FiberStatus fiber_status = FiberStatus::kSuccess;
  // GetLastError result if fiber conversion failed.
  DWORD fiber_error = ERROR_SUCCESS;
  if (!::IsThreadAFiber()) {
    // Make the main thread's stack size 4 MiB so that it has roughly the same
    // effective size as the 64-bit build's 8 MiB stack.
    constexpr size_t kStackSize = 4 * 1024 * 1024;  // 4 MiB
    // Leak the fiber on exit.
    LPVOID original_fiber =
        ::ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
    if (original_fiber) {
      FiberState fiber_state = {instance, original_fiber};
      // Create a fiber with a bigger stack and switch to it. Leak the fiber on
      // exit.
      LPVOID big_stack_fiber = ::CreateFiberEx(
          0, kStackSize, FIBER_FLAG_FLOAT_SWITCH, FiberBinder, &fiber_state);
      if (big_stack_fiber) {
        ::SwitchToFiber(big_stack_fiber);
        // The fibers must be cleaned up to avoid obscure TLS-related shutdown
        // crashes.
        ::DeleteFiber(big_stack_fiber);
        ::ConvertFiberToThread();
        // Control returns here after Chrome has finished running on FiberMain.
        return fiber_state.fiber_result;
      }
      fiber_status = FiberStatus::kCreateFiberFailed;
    } else {
      fiber_status = FiberStatus::kConvertFailed;
    }
    // If we reach here then creating and switching to a fiber has failed. This
    // probably means we are low on memory and will soon crash. Try to report
    // this error once crash reporting is initialized.
    fiber_error = ::GetLastError();
    base::debug::Alias(&fiber_error);
  }
  // If we are already a fiber then continue normal execution.
#endif  // defined(ARCH_CPU_32_BITS)

  struct Arguments {
    int argc = 0;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    ~Arguments() { LocalFree(argv); }
  } arguments;

  if (!arguments.argv)
    return -1;

#ifdef _DEBUG
  // Don't display assert dialog boxes in CI test runs
  static const char kCI[] = "CI";
  if (IsEnvSet(kCI)) {
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

    _set_error_mode(_OUT_TO_STDERR);
  }
#endif

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  bool run_as_node =
      electron::fuses::IsRunAsNodeEnabled() && IsEnvSet(electron::kRunAsNode);
#else
  bool run_as_node = false;
#endif

  // Make sure the output is printed to console.
  if (run_as_node || !IsEnvSet("ELECTRON_NO_ATTACH_CONSOLE"))
    base::RouteStdioToConsole(false);

  std::vector<char*> argv(arguments.argc);
  std::transform(arguments.argv, arguments.argv + arguments.argc, argv.begin(),
                 [](auto& a) { return _strdup(base::WideToUTF8(a).c_str()); });
#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  if (electron::fuses::IsRunAsNodeEnabled() && run_as_node) {
    base::AtExitManager atexit_manager;
    base::i18n::InitializeICU();
    auto ret = electron::NodeMain(argv.size(), argv.data());
    std::for_each(argv.begin(), argv.end(), free);
    return ret;
  }
#endif

  base::CommandLine::Init(argv.size(), argv.data());
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  const std::string process_type =
      command_line->GetSwitchValueASCII(kProcessType);

  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    // Check if we should monitor the exit code of this process
    std::unique_ptr<browser_watcher::ExitCodeWatcher> exit_code_watcher;

    // Retrieve the client process from the command line
    crashpad::InitialClientData initial_client_data;
    if (initial_client_data.InitializeFromString(
            command_line->GetSwitchValueASCII("initial-client-data"))) {
      // Setup exit code watcher to monitor the parent process
      HANDLE duplicate_handle = INVALID_HANDLE_VALUE;
      if (DuplicateHandle(
              ::GetCurrentProcess(), initial_client_data.client_process(),
              ::GetCurrentProcess(), &duplicate_handle,
              PROCESS_QUERY_INFORMATION, FALSE, DUPLICATE_SAME_ACCESS)) {
        base::Process parent_process(duplicate_handle);
        exit_code_watcher =
            std::make_unique<browser_watcher::ExitCodeWatcher>();
        if (exit_code_watcher->Initialize(std::move(parent_process))) {
          exit_code_watcher->StartWatching();
        }
      }
    }

    // The handler process must always be passed the user data dir on the
    // command line.
    DCHECK(command_line->HasSwitch(kUserDataDir));

    base::FilePath user_data_dir =
        command_line->GetSwitchValuePath(kUserDataDir);
    int crashpad_status = crash_reporter::RunAsCrashpadHandler(
        *command_line, user_data_dir, kProcessType, kUserDataDir);
    if (crashpad_status != 0 && exit_code_watcher) {
      // Crashpad failed to initialize, explicitly stop the exit code watcher
      // so the crashpad-handler process can exit with an error
      exit_code_watcher->StopWatching();
    }
    return crashpad_status;
  }

#if defined(ARCH_CPU_32_BITS)
  // Intentionally crash if converting to a fiber failed.
  CHECK_EQ(fiber_status, FiberStatus::kSuccess);
#endif  // defined(ARCH_CPU_32_BITS)

  if (!electron::CheckCommandLineArguments(arguments.argc, arguments.argv))
    return -1;

  sandbox::SandboxInterfaceInfo sandbox_info = {nullptr};
  content::InitializeSandboxInfo(&sandbox_info);
  electron::ElectronMainDelegate delegate;

  content::ContentMainParams params(&delegate);
  params.instance = instance;
  params.sandbox_info = &sandbox_info;
  electron::ElectronCommandLine::Init(arguments.argc, arguments.argv);
  return content::ContentMain(std::move(params));
}
