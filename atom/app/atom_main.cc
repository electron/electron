// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_main.h"

#include <stdlib.h>

#if defined(OS_WIN)
#include <windows.h>
#include <shellscalingapi.h>
#include <tchar.h>
#include <shellapi.h>

#include "atom/app/atom_main_delegate.h"
#include "atom/common/crash_reporter/win/crash_service_main.h"
#include "base/environment.h"
#include "base/win/windows_version.h"
#include "content/public/app/sandbox_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"
#include "ui/gfx/win/dpi.h"
#elif defined(OS_LINUX)  // defined(OS_WIN)
#include "atom/app/atom_main_delegate.h"  // NOLINT
#include "content/public/app/content_main.h"
#else  // defined(OS_LINUX)
#include "atom/app/atom_library_main.h"
#endif  // defined(OS_MACOSX)

#include "atom/app/node_main.h"
#include "atom/common/atom_command_line.h"
#include "base/at_exit.h"
#include "base/i18n/icu_util.h"

namespace {

const char* kRunAsNode = "ELECTRON_RUN_AS_NODE";
const char* kOldRunAsNode = "ATOM_SHELL_INTERNAL_RUN_AS_NODE";

bool IsEnvSet(const char* name) {
#if defined(OS_WIN)
  size_t required_size;
  getenv_s(&required_size, nullptr, 0, name);
  return required_size != 0;
#else
  char* indicator = getenv(name);
  return indicator && indicator[0] != '\0';
#endif
}

bool IsRunAsNode() {
  return IsEnvSet(kRunAsNode) || IsEnvSet(kOldRunAsNode);
}

}  // namespace

#if defined(OS_WIN)
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmd, int) {
  int argc = 0;
  wchar_t** wargv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

  // Make output work in console if we are not in cygiwn.
  if (!IsEnvSet("TERM") && !IsEnvSet("ELECTRON_NO_ATTACH_CONSOLE")) {
    AttachConsole(ATTACH_PARENT_PROCESS);

    FILE* dontcare;
    freopen_s(&dontcare, "CON", "w", stdout);
    freopen_s(&dontcare, "CON", "w", stderr);
  }

  // Convert argv to to UTF8
  char** argv = new char*[argc];
  for (int i = 0; i < argc; i++) {
    // Compute the size of the required buffer
    DWORD size = WideCharToMultiByte(CP_UTF8,
                                     0,
                                     wargv[i],
                                     -1,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL);
    if (size == 0) {
      // This should never happen.
      fprintf(stderr, "Could not convert arguments to utf8.");
      exit(1);
    }
    // Do the actual conversion
    argv[i] = new char[size];
    DWORD result = WideCharToMultiByte(CP_UTF8,
                                       0,
                                       wargv[i],
                                       -1,
                                       argv[i],
                                       size,
                                       NULL,
                                       NULL);
    if (result == 0) {
      // This should never happen.
      fprintf(stderr, "Could not convert arguments to utf8.");
      exit(1);
    }
  }

  if (IsRunAsNode()) {
    // Now that argv conversion is done, we can finally start.
    base::AtExitManager atexit_manager;
    base::i18n::InitializeICU();
    return atom::NodeMain(argc, argv);
  } else if (IsEnvSet("ATOM_SHELL_INTERNAL_CRASH_SERVICE")) {
    return crash_service::Main(cmd);
  }

  sandbox::SandboxInterfaceInfo sandbox_info = {0};
  content::InitializeSandboxInfo(&sandbox_info);
  atom::AtomMainDelegate delegate;

  content::ContentMainParams params(&delegate);
  params.instance = instance;
  params.sandbox_info = &sandbox_info;
  atom::AtomCommandLine::Init(argc, argv);
  return content::ContentMain(params);
}

#elif defined(OS_LINUX)  // defined(OS_WIN)

int main(int argc, const char* argv[]) {
  if (IsRunAsNode()) {
    base::i18n::InitializeICU();
    base::AtExitManager atexit_manager;
    return atom::NodeMain(argc, const_cast<char**>(argv));
  }

  atom::AtomMainDelegate delegate;
  content::ContentMainParams params(&delegate);
  params.argc = argc;
  params.argv = argv;
  atom::AtomCommandLine::Init(argc, argv);
  return content::ContentMain(params);
}

#else  // defined(OS_LINUX)

int main(int argc, const char* argv[]) {
  if (IsRunAsNode()) {
    return AtomInitializeICUandStartNode(argc, const_cast<char**>(argv));
  }

  return AtomMain(argc, argv);
}

#endif  // defined(OS_MACOSX)
