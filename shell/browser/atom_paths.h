// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ATOM_PATHS_H_
#define SHELL_BROWSER_ATOM_PATHS_H_

#include "shell/browser/electron_paths.h"

namespace base {
class FilePath;
}

namespace electron {

class AtomPaths {
 public:
  static void Register();

  static bool GetDefault(int key, base::FilePath* path);
};

}  // namespace electron

#endif  // SHELL_BROWSER_ATOM_PATHS_H_
