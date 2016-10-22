// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_API_RENDERER_IPC_H_
#define ATOM_RENDERER_API_ATOM_API_RENDERER_IPC_H_

#include "base/values.h"
#include "native_mate/arguments.h"

namespace atom {

namespace api {

void Send(mate::Arguments* args,
          const base::string16& channel,
          const base::ListValue& arguments);

base::string16 SendSync(mate::Arguments* args,
                        const base::string16& channel,
                        const base::ListValue& arguments);

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv);

}  // namespace api

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_API_RENDERER_IPC_H_
