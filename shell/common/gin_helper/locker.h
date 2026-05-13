// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_LOCKER_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_LOCKER_H_

#include <optional>

#include "v8/include/v8-locker.h"

namespace v8 {
class Isolate;
}  // namespace v8

namespace gin_helper {

// Only lock when lockers are used in current thread.
class Locker {
 public:
  explicit Locker(v8::Isolate* isolate);
  ~Locker();

  // disable copy
  Locker(const Locker&) = delete;
  Locker& operator=(const Locker&) = delete;

  // prevent heap allocation
  void* operator new(size_t size) = delete;
  void operator delete(void*, size_t) = delete;

 private:
  // Inline storage — avoids a heap allocation per C++ -> JS callback.
  std::optional<v8::Locker> locker_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_LOCKER_H_
