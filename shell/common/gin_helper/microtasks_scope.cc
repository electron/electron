// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/microtasks_scope.h"

#include "shell/common/process_util.h"
#include "v8/include/v8-context.h"

namespace gin_helper {

MicrotasksScope::MicrotasksScope(v8::Local<v8::Context> context,
                                 bool ignore_browser_checkpoint,
                                 v8::MicrotasksScope::Type scope_type) {
  auto* isolate = context->GetIsolate();
  auto* microtask_queue = context->GetMicrotaskQueue();
  if (electron::IsBrowserProcess()) {
    if (!ignore_browser_checkpoint)
      v8::MicrotasksScope::PerformCheckpoint(isolate);
  } else {
    v8_microtasks_scope_.emplace(isolate, microtask_queue, scope_type);
  }
}

MicrotasksScope::~MicrotasksScope() = default;

}  // namespace gin_helper
