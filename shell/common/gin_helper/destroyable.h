// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_HELPER_DESTROYABLE_H_
#define SHELL_COMMON_GIN_HELPER_DESTROYABLE_H_

#include "v8/include/v8.h"

namespace gin_helper {

// Used by gin helpers to destroy native objects.
struct Destroyable {
  static void Destroy(const v8::FunctionCallbackInfo<v8::Value>& info);
  static bool IsDestroyed(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void MakeDestroyable(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype);
};

}  // namespace gin_helper

#endif  // SHELL_COMMON_GIN_HELPER_DESTROYABLE_H_
