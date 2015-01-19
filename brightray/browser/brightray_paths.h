// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_BRIGHTRAY_PATHS_H_
#define BROWSER_BRIGHTRAY_PATHS_H_

#include "base/compiler_specific.h"

#if defined(OS_WIN)
#include "base/base_paths_win.h"
#elif defined(OS_MACOSX)
#include "base/base_paths_mac.h"
#endif

namespace brightray {

enum {
  PATH_START = 1000,

  DIR_USER_DATA = PATH_START,  // Directory where user data can be written.

#if defined(OS_LINUX)
  DIR_APP_DATA,  // Application Data directory under the user profile.
#else
  DIR_APP_DATA = base::DIR_APP_DATA,
#endif

  PATH_END
};

}  // namespace brightray

#endif  // BROWSER_BRIGHTRAY_PATHS_H_
