// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef NATIVE_MATE_NATIVE_MATE_MICROTASKS_SCOPE_H_
#define NATIVE_MATE_NATIVE_MATE_MICROTASKS_SCOPE_H_

#include <memory>

#include "base/macros.h"
#include "v8/include/v8.h"

namespace mate {

// In the browser process runs v8::MicrotasksScope::PerformCheckpoint
// In the render process creates a v8::MicrotasksScope.
class MicrotasksScope {
 public:
  explicit MicrotasksScope(v8::Isolate* isolate,
                           bool ignore_browser_checkpoint = false);
  ~MicrotasksScope();

 private:
  std::unique_ptr<v8::MicrotasksScope> v8_microtasks_scope_;

  DISALLOW_COPY_AND_ASSIGN(MicrotasksScope);
};

}  // namespace mate

#endif  // NATIVE_MATE_NATIVE_MATE_MICROTASKS_SCOPE_H_
