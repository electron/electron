// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_LOCKER_H_
#define NATIVE_MATE_LOCKER_H_

#include "base/memory/scoped_ptr.h"
#include "v8/include/v8.h"

namespace mate {

// Only lock when lockers are used in current thread.
class Locker {
 public:
  explicit Locker(v8::Isolate* isolate);
  ~Locker();

 private:
  void* operator new(size_t size);
  void operator delete(void*, size_t);

  scoped_ptr<v8::Locker> locker_;

  DISALLOW_COPY_AND_ASSIGN(Locker);
};

}  // namespace mate

#endif  // NATIVE_MATE_LOCKER_H_
