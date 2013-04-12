// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app/atom_library_main.h"

#include "app/atom_main_delegate.h"
#include "content/public/app/content_main.h"

int AtomMain(int argc, const char* argv[]) {
  atom::AtomMainDelegate delegate;
  return content::ContentMain(argc, argv, &delegate);
}
