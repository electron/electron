// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/microtasks_scope.h"

#include "shell/common/process_util.h"

namespace gin_helper {

MicrotasksScope::MicrotasksScope(v8::Isolate* isolate,
                                 v8::MicrotaskQueue* microtask_queue,
                                 bool ignore_browser_checkpoint,
                                 v8::MicrotasksScope::Type scope_type) {
  if (electron::IsBrowserProcess()) {
    if (!ignore_browser_checkpoint)
      v8::MicrotasksScope::PerformCheckpoint(isolate);
  } else {
    v8_microtasks_scope_ = std::make_unique<v8::MicrotasksScope>(
        isolate, microtask_queue, scope_type);
  }
}

MicrotasksScope::~MicrotasksScope() = default;

}  // namespace gin_helper
