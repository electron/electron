// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_BRIGHTRAY_PATHS_H_
#define BROWSER_BRIGHTRAY_PATHS_H_

namespace brightray {

enum {
  PATH_START = 1000,

  DIR_USER_DATA = PATH_START,  // Directory where user data can be written.

  PATH_END
};

}  // namespace brightray

#endif  // BROWSER_BRIGHTRAY_PATHS_H_
