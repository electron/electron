// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_BRIGHTRAY_PATHS_H_
#define BRIGHTRAY_BROWSER_BRIGHTRAY_PATHS_H_

#if defined(OS_WIN)
#include "base/base_paths_win.h"
#elif defined(OS_MACOSX)
#include "base/base_paths_mac.h"
#endif

#if defined(OS_POSIX)
#include "base/base_paths_posix.h"
#endif

namespace brightray {

enum {
  PATH_START = 11000,

  DIR_USER_DATA = PATH_START,  // Directory where user data can be written.
  DIR_USER_CACHE,  // Directory where user cache can be written.
  DIR_APP_LOGS,  // Directory where app logs live

#if defined(OS_LINUX)
  DIR_APP_DATA,  // Application Data directory under the user profile.
#else
  DIR_APP_DATA = base::DIR_APP_DATA,
#endif

#if defined(OS_POSIX)
  DIR_CACHE = base::DIR_CACHE,  // Directory where to put cache data.
#else
  DIR_CACHE = base::DIR_APP_DATA,
#endif

  PATH_END
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_BRIGHTRAY_PATHS_H_
