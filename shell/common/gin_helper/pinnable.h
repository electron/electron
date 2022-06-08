// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_PINNABLE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_PINNABLE_H_

#include "v8/include/v8.h"

namespace gin_helper {

template <typename T>
class Pinnable {
 protected:
  // Prevent the object from being garbage collected until Unpin() is called.
  void Pin(v8::Isolate* isolate) {
    v8::HandleScope scope(isolate);
    v8::Local<v8::Value> wrapper;
    if (static_cast<T*>(this)->GetWrapper(isolate).ToLocal(&wrapper)) {
      pinned_.Reset(isolate, wrapper);
    }
  }

  // Allow the object to be garbage collected.
  void Unpin() { pinned_.Reset(); }

 private:
  v8::Global<v8::Value> pinned_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_PINNABLE_H_
