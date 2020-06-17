// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "native_mate/microtasks_scope.h"

namespace mate {

MicrotasksScope::MicrotasksScope(v8::Isolate* isolate,
                                 bool ignore_browser_checkpoint) {
  if (v8::Locker::IsActive()) {
    if (!ignore_browser_checkpoint)
      v8::MicrotasksScope::PerformCheckpoint(isolate);
  } else {
    v8_microtasks_scope_ = std::make_unique<v8::MicrotasksScope>(
        isolate, v8::MicrotasksScope::kRunMicrotasks);
  }
}

MicrotasksScope::~MicrotasksScope() = default;

}  // namespace mate
