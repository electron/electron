// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_MICROTASKS_SCOPE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_MICROTASKS_SCOPE_H_

#include <memory>

#include "v8/include/v8.h"

namespace gin_helper {

// In the browser process runs v8::MicrotasksScope::PerformCheckpoint
// In the render process creates a v8::MicrotasksScope.
class MicrotasksScope {
 public:
  explicit MicrotasksScope(v8::Isolate* isolate,
                           v8::MicrotaskQueue* microtask_queue,
                           bool ignore_browser_checkpoint = false,
                           v8::MicrotasksScope::Type scope_type =
                               v8::MicrotasksScope::kRunMicrotasks);
  ~MicrotasksScope();

  // disable copy
  MicrotasksScope(const MicrotasksScope&) = delete;
  MicrotasksScope& operator=(const MicrotasksScope&) = delete;

 private:
  std::unique_ptr<v8::MicrotasksScope> v8_microtasks_scope_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_MICROTASKS_SCOPE_H_
