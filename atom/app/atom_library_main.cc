// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_library_main.h"

#include "atom/app/atom_main_delegate.h"
#include "content/public/app/content_main.h"

#if defined(OS_MACOSX)
int AtomMain(int argc, const char* argv[]) {
  atom::AtomMainDelegate delegate;
  return content::ContentMain(argc, argv, &delegate);
}
#endif  // OS_MACOSX
