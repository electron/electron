// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_ELECTRON_PATHS_H_
#define ELECTRON_SHELL_COMMON_ELECTRON_PATHS_H_

#include "base/base_paths.h"

#if BUILDFLAG(IS_WIN)
#include "base/base_paths_win.h"
#elif BUILDFLAG(IS_MAC)
#include "base/base_paths_mac.h"
#endif

#if BUILDFLAG(IS_POSIX)
#include "base/base_paths_posix.h"
#endif

namespace electron {

enum {
  PATH_START = 11000,

  DIR_USER_CACHE = PATH_START,  // Directory where user cache can be written.
  DIR_APP_LOGS,                 // Directory where app logs live

#if BUILDFLAG(IS_WIN)
  DIR_RECENT,  // Directory where recent files live
#endif

#if BUILDFLAG(IS_LINUX)
  DIR_APP_DATA,  // Application Data directory under the user profile.
#endif

  DIR_CRASH_DUMPS,  // c.f. chrome::DIR_CRASH_DUMPS

  PATH_END,  // End of new paths. Those that follow redirect to base::DIR_*

#if BUILDFLAG(IS_WIN)
  DIR_APP_DATA = base::DIR_ROAMING_APP_DATA,
#elif BUILDFLAG(IS_MAC)
  DIR_APP_DATA = base::DIR_APP_DATA,
#endif
};

static_assert(PATH_START < PATH_END, "invalid PATH boundaries");

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_ELECTRON_PATHS_H_
