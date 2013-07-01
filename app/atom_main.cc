// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/app/content_main.h"

#if defined(OS_WIN)

#include "app/atom_main_delegate.h"
#include "content/public/app/startup_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int) {
  sandbox::SandboxInterfaceInfo sandbox_info = {0};
  content::InitializeSandboxInfo(&sandbox_info);
  atom::AtomMainDelegate delegate;
  return content::ContentMain(instance, &sandbox_info, &delegate);
}

#else

#include "app/atom_library_main.h"

int main(int argc, const char* argv[]) {
  return AtomMain(argc, argv);
}

#endif  // OS_WIN
