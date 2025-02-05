// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_PRELOAD_UTILS_H_
#define ELECTRON_SHELL_RENDERER_PRELOAD_UTILS_H_

#include "v8/include/v8-forward.h"

namespace gin_helper {
class Arguments;
}

namespace electron::preload_utils {

v8::Local<v8::Value> GetBinding(v8::Isolate* isolate,
                                v8::Local<v8::String> key,
                                gin_helper::Arguments* margs);

v8::Local<v8::Value> CreatePreloadScript(v8::Isolate* isolate,
                                         v8::Local<v8::String> source);

double Uptime();

}  // namespace electron::preload_utils

#endif  // ELECTRON_SHELL_RENDERER_PRELOAD_UTILS_H_
