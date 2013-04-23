// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_API_RENDERER_IPC_H_
#define ATOM_RENDERER_API_ATOM_API_RENDERER_IPC_H_

#include "base/basictypes.h"
#include "v8/include/v8.h"

namespace atom {

namespace api {

class RendererIPC {
 public:
  static void Initialize(v8::Handle<v8::Object> target);

 private:
  static v8::Handle<v8::Value> Send(const v8::Arguments &args);

  DISALLOW_IMPLICIT_CONSTRUCTORS(RendererIPC);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_API_RENDERER_IPC_H_
