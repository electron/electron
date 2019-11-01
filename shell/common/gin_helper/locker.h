// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef SHELL_COMMON_GIN_HELPER_LOCKER_H_
#define SHELL_COMMON_GIN_HELPER_LOCKER_H_

#include <memory>

#include "base/macros.h"
#include "v8/include/v8.h"

namespace gin_helper {

// Only lock when lockers are used in current thread.
class Locker {
 public:
  explicit Locker(v8::Isolate* isolate);
  ~Locker();

  // Returns whether current process is browser process, currently we detect it
  // by checking whether current has used V8 Lock, but it might be a bad idea.
  static inline bool IsBrowserProcess() { return v8::Locker::IsActive(); }

 private:
  void* operator new(size_t size);
  void operator delete(void*, size_t);

  std::unique_ptr<v8::Locker> locker_;

  DISALLOW_COPY_AND_ASSIGN(Locker);
};

}  // namespace gin_helper

#endif  // SHELL_COMMON_GIN_HELPER_LOCKER_H_
