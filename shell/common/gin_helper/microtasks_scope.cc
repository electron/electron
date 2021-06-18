// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/microtasks_scope.h"

#include "shell/common/gin_helper/locker.h"

namespace gin_helper {

MicrotasksScope::MicrotasksScope(v8::Isolate* isolate,
                                 bool ignore_browser_checkpoint,
                                 v8::MicrotasksScope::Type scope_type) {
  if (Locker::IsBrowserProcess()) {
    if (!ignore_browser_checkpoint)
      v8::MicrotasksScope::PerformCheckpoint(isolate);
  } else {
    v8_microtasks_scope_ =
        std::make_unique<v8::MicrotasksScope>(isolate, scope_type);
  }
}

MicrotasksScope::~MicrotasksScope() = default;

}  // namespace gin_helper
