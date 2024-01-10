
// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/microtasks_runner.h"

#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

namespace electron {

MicrotasksRunner::MicrotasksRunner(v8::Isolate* isolate) : isolate_(isolate) {}

void MicrotasksRunner::WillProcessTask(const base::PendingTask& pending_task,
                                       bool was_blocked_or_low_priority) {}

void MicrotasksRunner::DidProcessTask(const base::PendingTask& pending_task) {
  v8::Isolate::Scope scope(isolate_);
  // In the browser process we follow Node.js microtask policy of kExplicit
  // and let the MicrotaskRunner which is a task observer for chromium UI thread
  // scheduler run the microtask checkpoint. This worked fine because Node.js
  // also runs microtasks through its task queue, but after
  // https://github.com/electron/electron/issues/20013 Node.js now performs its
  // own microtask checkpoint and it may happen is some situations that there is
  // contention for performing checkpoint between Node.js and chromium, ending
  // up Node.js delaying its callbacks. To fix this, now we always lets Node.js
  // handle the checkpoint in the browser process.
  {
    v8::HandleScope handle_scope(isolate_);
    node::CallbackScope microtasks_scope(isolate_, v8::Object::New(isolate_),
                                         {0, 0});
  }
}

}  // namespace electron
