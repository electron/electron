// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_DESTROYABLE_H_
#define SHELL_COMMON_GIN_DESTROYABLE_H_

#include "native_mate/arguments.h"

namespace gin {

// Used by gin helpers to destroy native objects.
struct Destroyable {
  static void Destroy(mate::Arguments* args);
  static bool IsDestroyed(mate::Arguments* args);
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_DESTROYABLE_H_
