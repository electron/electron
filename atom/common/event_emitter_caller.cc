// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/event_emitter_caller.h"

#include "atom/common/node_includes.h"

namespace mate {

namespace internal {

v8::Local<v8::Value> CallEmitWithArgs(v8::Isolate* isolate,
                                      v8::Local<v8::Object> obj,
                                      ValueVector* args) {
  return node::MakeCallback(
      isolate, obj, "emit", args->size(), &args->front());
}

}  // namespace internal

}  // namespace mate
