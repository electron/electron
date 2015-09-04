// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_BRIDGE_TASK_RUNNER_H_
#define ATOM_BROWSER_BRIDGE_TASK_RUNNER_H_

#include "base/single_thread_task_runner.h"

namespace atom {

// Post all tasks to the current message loop's task runner if available,
// otherwise fail silently.
class BridgeTaskRunner : public base::SingleThreadTaskRunner {
 public:
  BridgeTaskRunner() {}
  ~BridgeTaskRunner() override {}

  // base::SingleThreadTaskRunner:
  bool PostDelayedTask(const tracked_objects::Location& from_here,
                       const base::Closure& task,
                       base::TimeDelta delay) override;
  bool RunsTasksOnCurrentThread() const override;
  bool PostNonNestableDelayedTask(
      const tracked_objects::Location& from_here,
      const base::Closure& task,
      base::TimeDelta delay) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BridgeTaskRunner);
};

}  // namespace atom

#endif  // ATOM_BROWSER_BRIDGE_TASK_RUNNER_H_
