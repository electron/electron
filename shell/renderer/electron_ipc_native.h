// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_ELECTRON_IPC_NATIVE_H_
#define ELECTRON_SHELL_RENDERER_ELECTRON_IPC_NATIVE_H_

#include <vector>

#include "v8/include/v8-forward.h"

namespace electron::ipc_native {

void EmitIPCEvent(const v8::Local<v8::Context>& context,
                  bool internal,
                  const std::string& channel,
                  std::vector<v8::Local<v8::Value>> ports,
                  v8::Local<v8::Value> args);

}  // namespace electron::ipc_native

#endif  // ELECTRON_SHELL_RENDERER_ELECTRON_IPC_NATIVE_H_
