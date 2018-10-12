// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_MICROTASKS_RUNNER_H_
#define ATOM_BROWSER_MICROTASKS_RUNNER_H_

#include "base/message_loop/message_loop.h"

namespace v8 {
class Isolate;
}

namespace atom {

// Microtasks like promise resolution, are run at the end of the current
// task. This class implements a task observer that runs tells v8 to run them.
class MicrotasksRunner : public base::MessageLoop::TaskObserver {
 public:
  explicit MicrotasksRunner(v8::Isolate* isolate);

  // base::MessageLoop::TaskObserver
  void WillProcessTask(const base::PendingTask& pending_task) override;
  void DidProcessTask(const base::PendingTask& pending_task) override;

 private:
  v8::Isolate* isolate_;
};

}  // namespace atom

#endif  // ATOM_BROWSER_MICROTASKS_RUNNER_H_
