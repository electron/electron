// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_EVENT_EMITTER_CALLER_H_
#define ATOM_COMMON_EVENT_EMITTER_CALLER_H_

#include <vector>

#include "native_mate/converter.h"

namespace mate {

namespace internal {

using ValueVector = std::vector<v8::Local<v8::Value>>;

v8::Local<v8::Value> CallEmitWithArgs(v8::Isolate* isolate,
                                      v8::Local<v8::Object> obj,
                                      ValueVector* args);

}  // namespace internal

// obj.emit(name, args...);
// The caller is responsible of allocating a HandleScope.
template<typename... Args>
v8::Local<v8::Value> EmitEvent(v8::Isolate* isolate,
                               v8::Local<v8::Object> obj,
                               const base::StringPiece& name,
                               const Args&... args) {
  internal::ValueVector converted_args = {
      ConvertToV8(isolate, name),
      ConvertToV8(isolate, args)...,
  };
  return internal::CallEmitWithArgs(isolate, obj, &converted_args);
}

}  // namespace mate

#endif  // ATOM_COMMON_EVENT_EMITTER_CALLER_H_
