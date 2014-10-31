// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_main.h"

#include <stdlib.h>
#include <string.h>

#if defined(OS_WIN)
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#include <windows.h>
#include <shellapi.h>

#include "atom/app/atom_main_delegate.h"
#include "atom/common/crash_reporter/win/crash_service_main.h"
#include "base/environment.h"
#include "content/public/app/startup_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"
#include "ui/gfx/win/dpi.h"
#elif defined(OS_LINUX)  // defined(OS_WIN)
#include "atom/app/atom_main_delegate.h"  // NOLINT
#include "content/public/app/content_main.h"
#else  // defined(OS_LINUX)
#include "atom/app/atom_library_main.h"
#endif  // defined(OS_MACOSX)

// Declaration of node::Start.
namespace node {
int Start(int argc, char *argv[]);
}

#if defined(OS_WIN)

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmd, int) {
  int argc = 0;
  wchar_t** wargv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

  scoped_ptr<base::Environment> env(base::Environment::Create());

  // Make output work in console if we are not in cygiwn.
  std::string os;
  if (env->GetVar("OS", &os) && os != "cygwin") {
    AttachConsole(ATTACH_PARENT_PROCESS);

    FILE* dontcare;
    freopen_s(&dontcare, "CON", "w", stdout);
    freopen_s(&dontcare, "CON", "w", stderr);
    freopen_s(&dontcare, "CON", "r", stdin);
  }

  std::string node_indicator, crash_service_indicator;
  if (env->GetVar("ATOM_SHELL_INTERNAL_RUN_AS_NODE", &node_indicator) &&
      node_indicator == "1") {
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
    // Now that conversion is done, we can finally start.
    return node::Start(argc, argv);
  } else if (env->GetVar("ATOM_SHELL_INTERNAL_CRASH_SERVICE",
                         &crash_service_indicator) &&
      crash_service_indicator == "1") {
    return crash_service::Main(cmd);
  }

  sandbox::SandboxInterfaceInfo sandbox_info = {0};
  content::InitializeSandboxInfo(&sandbox_info);
  atom::AtomMainDelegate delegate;

  // Now chrome relies on a regkey to enable high dpi support.
  gfx::EnableHighDPISupport();

  content::ContentMainParams params(&delegate);
  params.instance = instance;
  params.sandbox_info = &sandbox_info;
  return content::ContentMain(params);
}

#elif defined(OS_LINUX)  // defined(OS_WIN)

int main(int argc, const char* argv[]) {
  char* node_indicator = getenv("ATOM_SHELL_INTERNAL_RUN_AS_NODE");
  if (node_indicator != NULL && strcmp(node_indicator, "1") == 0)
    return node::Start(argc, const_cast<char**>(argv));

  atom::AtomMainDelegate delegate;
  content::ContentMainParams params(&delegate);
  params.argc = argc;
  params.argv = argv;
  return content::ContentMain(params);
}

#else  // defined(OS_LINUX)

int main(int argc, const char* argv[]) {
  char* node_indicator = getenv("ATOM_SHELL_INTERNAL_RUN_AS_NODE");
  if (node_indicator != NULL && strcmp(node_indicator, "1") == 0)
    return node::Start(argc, const_cast<char**>(argv));

  return AtomMain(argc, argv);
}

#endif  // defined(OS_MACOSX)
