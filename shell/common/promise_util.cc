// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/promise_util.h"

namespace mate {

template <typename T>
v8::Local<v8::Value> mate::Converter<electron::util::Promise<T>>::ToV8(
    v8::Isolate*,
    const electron::util::Promise<T>& val) {
  return val.GetHandle();
}

template <>
v8::Local<v8::Value> mate::Converter<electron::util::Promise<void*>>::ToV8(
    v8::Isolate*,
    const electron::util::Promise<void*>& val) {
  return val.GetHandle();
}

}  // namespace mate
