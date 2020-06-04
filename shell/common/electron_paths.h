// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_ELECTRON_PATHS_H_
#define SHELL_COMMON_ELECTRON_PATHS_H_

#include "base/base_paths.h"

#if defined(OS_WIN)
#include "base/base_paths_win.h"
#elif defined(OS_MACOSX)
#include "base/base_paths_mac.h"
#endif

#if defined(OS_POSIX)
#include "base/base_paths_posix.h"
#endif

namespace electron {

enum {
  PATH_START = 11000,

  DIR_USER_DATA = PATH_START,  // Directory where user data can be written.
  DIR_USER_CACHE,              // Directory where user cache can be written.
  DIR_APP_LOGS,                // Directory where app logs live

#if defined(OS_WIN)
  DIR_RECENT,  // Directory where recent files live
#endif

#if defined(OS_LINUX)
  DIR_APP_DATA,  // Application Data directory under the user profile.
#endif

  DIR_CRASH_DUMPS,  // c.f. chrome::DIR_CRASH_DUMPS

  PATH_END,  // End of new paths. Those that follow redirect to base::DIR_*

#if !defined(OS_LINUX)
  DIR_APP_DATA = base::DIR_APP_DATA,
#endif

#if defined(OS_POSIX)
  DIR_CACHE = base::DIR_CACHE  // Directory where to put cache data.
#else
  DIR_CACHE = base::DIR_APP_DATA
#endif
};

static_assert(PATH_START < PATH_END, "invalid PATH boundaries");

}  // namespace electron

#endif  // SHELL_COMMON_ELECTRON_PATHS_H_
