
// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/microtasks_runner.h"
#include "v8/include/v8.h"

namespace electron {

MicrotasksRunner::MicrotasksRunner(v8::Isolate* isolate) : isolate_(isolate) {}

void MicrotasksRunner::WillProcessTask(const base::PendingTask& pending_task,
                                       bool was_blocked_or_low_priority) {}

void MicrotasksRunner::DidProcessTask(const base::PendingTask& pending_task) {
  v8::Isolate::Scope scope(isolate_);
  v8::MicrotasksScope::PerformCheckpoint(isolate_);
}

}  // namespace electron
