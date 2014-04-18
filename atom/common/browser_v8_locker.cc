// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/common/browser_v8_locker.h"

namespace node {
extern bool g_standalone_mode;
}

namespace atom {

BrowserV8Locker::BrowserV8Locker(v8::Isolate* isolate) {
  if (node::g_standalone_mode)
    locker_.reset(new v8::Locker(isolate));
}

BrowserV8Locker::~BrowserV8Locker() {
}

}  // namespace atom
