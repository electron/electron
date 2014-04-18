// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_BROWSER_V8_LOCKER_H_
#define ATOM_COMMON_BROWSER_V8_LOCKER_H_

#include "base/memory/scoped_ptr.h"
#include "v8/include/v8.h"

namespace atom {

// Like v8::Locker, but only do lock when in browser process.
class BrowserV8Locker {
 public:
  explicit BrowserV8Locker(v8::Isolate* isolate);
  ~BrowserV8Locker();

 private:
  void* operator new(size_t size);
  void operator delete(void*, size_t);

  scoped_ptr<v8::Locker> locker_;
};

}  // namespace atom

#endif  // ATOM_COMMON_BROWSER_V8_LOCKER_H_
