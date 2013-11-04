// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#include "content/public/app/content_main.h"

namespace node {
int Start(int argc, char *argv[]);
}

#if defined(OS_WIN)

#include <windows.h>  // NOLINT
#include <shellapi.h>  // NOLINT

#include "app/atom_main_delegate.h"
#include "base/environment.h"
#include "content/public/app/startup_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"

/*
 * An alternate way to skin the same cat

void ConnectStdioToConsole()
{
  int hCrt;
  AllocConsole();

  HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
  hCrt = _open_osfhandle((long) handle_out, _O_TEXT);
  FILE* hf_out = _fdopen(hCrt, "w");
  setvbuf(hf_out, NULL, _IONBF, 1);
  *stdout = *hf_out;

  HANDLE handle_err = GetStdHandle(STD_ERROR_HANDLE);
  hCrt = _open_osfhandle((long) handle_err, _O_TEXT);
  FILE* hf_err = _fdopen(hCrt, "w");
  setvbuf(hf_err, NULL, _IONBF, 1);
  *stderr = *hf_err;


  HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
  hCrt = _open_osfhandle((long) handle_in, _O_TEXT);
  FILE* hf_in = _fdopen(hCrt, "r");
  setvbuf(hf_in, NULL, _IONBF, 128);
  *stdin = *hf_in;
}
*/

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmd, int) {
  int argc = 0;
  wchar_t** wargv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

  // Attach to the parent console if we've got one so that stdio works
  if (GetConsoleWindow()) {
    AllocConsole() ;
    AttachConsole(GetCurrentProcessId());
    freopen("CON", "w", stdout);
    freopen("CON", "w", stderr);
    freopen("CON", "r", stdin);
  }

  scoped_ptr<base::Environment> env(base::Environment::Create());
  std::string node_indicator;
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
  }

  sandbox::SandboxInterfaceInfo sandbox_info = {0};
  content::InitializeSandboxInfo(&sandbox_info);
  atom::AtomMainDelegate delegate;
  return content::ContentMain(instance, &sandbox_info, &delegate);
}

#else  // defined(OS_WIN)

#include "app/atom_library_main.h"

int main(int argc, const char* argv[]) {
  char* node_indicator = getenv("ATOM_SHELL_INTERNAL_RUN_AS_NODE");
  if (node_indicator != NULL && strcmp(node_indicator, "1") == 0)
    return node::Start(argc, const_cast<char**>(argv));

  return AtomMain(argc, argv);
}

#endif
