// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_BRIDGE_TASK_RUNNER_H_
#define ATOM_BROWSER_BRIDGE_TASK_RUNNER_H_

#include <vector>

#include "base/single_thread_task_runner.h"
#include "base/tuple.h"

namespace atom {

// Post all tasks to the current message loop's task runner if available,
// otherwise delay the work until message loop is ready.
class BridgeTaskRunner : public base::SingleThreadTaskRunner {
 public:
  BridgeTaskRunner() {}
  ~BridgeTaskRunner() override {}

  // Called when message loop is ready.
  static void MessageLoopIsReady();

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
  using TaskPair = base::Tuple<
      tracked_objects::Location, base::Closure, base::TimeDelta>;
  static std::vector<TaskPair> tasks_;
  static std::vector<TaskPair> non_nestable_tasks_;

  DISALLOW_COPY_AND_ASSIGN(BridgeTaskRunner);
};

}  // namespace atom

#endif  // ATOM_BROWSER_BRIDGE_TASK_RUNNER_H_
