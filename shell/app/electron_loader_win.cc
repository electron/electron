// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <windows.h>  // windows.h must be included first

#include <memory>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/threading/platform_thread.h"
#include "base/win/pe_image.h"
#include "content/public/app/sandbox_helper_win.h"
#include "electron/fuses.h"
#include "sandbox/win/src/sandbox_types.h"

namespace {

class RuntimePreReader : public base::PlatformThread::Delegate {
 public:
  RuntimePreReader(HMODULE runtime, SIZE_T size)
      : runtime_(runtime), size_(size) {}

  ~RuntimePreReader() override = default;

  void ThreadMain() override {
    ::_WIN32_MEMORY_RANGE_ENTRY range = {runtime_, size_};
    ::PrefetchVirtualMemory(::GetCurrentProcess(), 1, &range, 0);
    delete this;
  }

 private:
  HMODULE runtime_;
  SIZE_T size_;
};

}  // namespace

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int) {
  base::CommandLine::Init(0, nullptr);

  base::FilePath executable_dir;
  if (!base::PathService::Get(base::DIR_EXE, &executable_dir))
    return ERROR_PATH_NOT_FOUND;
  const base::FilePath runtime_path =
      executable_dir.Append(FILE_PATH_LITERAL("main.dll"));

  sandbox::SandboxInterfaceInfo sandbox_info = {nullptr};
  content::InitializeSandboxInfo(&sandbox_info);

  // Keep the runtime loaded through CRT shutdown, including addon destructors.
  HMODULE runtime = ::LoadLibraryExW(
      runtime_path.value().c_str(), nullptr,
      LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
  if (!runtime) {
    DWORD error = ::GetLastError();
    PLOG(ERROR) << "Unable to load Electron runtime";
    return static_cast<int>(error);
  }

  using MainEntry =
      int (*)(HINSTANCE, sandbox::SandboxInterfaceInfo*, const volatile char*);
  auto main_entry =
      reinterpret_cast<MainEntry>(::GetProcAddress(runtime, "ElectronMain"));
  if (!main_entry) {
    DWORD error = ::GetLastError();
    PLOG(ERROR) << "Unable to find Electron runtime entry point";
    return static_cast<int>(error);
  }

  // Preread the loaded image without mapping a second copy of main.dll. Keep
  // this work off the browser startup thread and below its CPU/I/O priority.
  if (base::CommandLine::ForCurrentProcess()
          ->GetSwitchValueASCII("type")
          .empty() &&
      !base::Environment::Create()->HasVar("ELECTRON_RUN_AS_NODE")) {
    const SIZE_T size =
        base::win::PEImage(runtime).GetNTHeaders()->OptionalHeader.SizeOfImage;
    if (size != 0) {
      std::unique_ptr<RuntimePreReader> reader =
          std::make_unique<RuntimePreReader>(runtime, size);
      if (base::PlatformThread::CreateNonJoinableWithType(
              0, reader.get(), base::ThreadType::kBackground)) {
        reader.release();
      }
    }
  }

  return main_entry(instance, &sandbox_info, electron::fuses::kFuseWire);
}
