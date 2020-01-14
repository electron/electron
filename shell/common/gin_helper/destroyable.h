// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_HELPER_DESTROYABLE_H_
#define SHELL_COMMON_GIN_HELPER_DESTROYABLE_H_

#include "v8/include/v8.h"

namespace gin_helper {

// Manage the native object wrapped in JS wrappers.
struct Destroyable {
  // Determine whether the native object has been destroyed.
  static bool IsDestroyed(v8::Local<v8::Object> object);

  // Add "destroy" and "isDestroyed" to prototype chain.
  static void MakeDestroyable(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype);
};

}  // namespace gin_helper

#endif  // SHELL_COMMON_GIN_HELPER_DESTROYABLE_H_
