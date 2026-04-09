// Copyright (c) 2025 Reito <reito@chromium.org>
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_API_ELECTRON_API_SHARED_TEXTURE_H_
#define ELECTRON_SHELL_COMMON_API_ELECTRON_API_SHARED_TEXTURE_H_

#include <string>
#include <vector>

#include "v8/include/v8-forward.h"

namespace electron::api::shared_texture {

v8::Local<v8::Value> ImportSharedTexture(v8::Isolate* isolate,
                                         v8::Local<v8::Value> options);

v8::Local<v8::Value> FinishTransferSharedTexture(v8::Isolate* isolate,
                                                 v8::Local<v8::Value> options);

}  // namespace electron::api::shared_texture

#endif  // ELECTRON_SHELL_COMMON_API_ELECTRON_API_SHARED_TEXTURE_H_
