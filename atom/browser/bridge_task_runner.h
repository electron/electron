// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_BRIDGE_TASK_RUNNER_H_
#define ATOM_BROWSER_BRIDGE_TASK_RUNNER_H_

#include <tuple>
#include <vector>

#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/tuple.h"

namespace atom {

// Post all tasks to the current message loop's task runner if available,
// otherwise delay the work until message loop is ready.
class BridgeTaskRunner : public base::SingleThreadTaskRunner {
 public:
  BridgeTaskRunner();

  // Called when message loop is ready.
  void MessageLoopIsReady();

  // base::SingleThreadTaskRunner:
  bool PostDelayedTask(const base::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override;
  bool RunsTasksInCurrentSequence() const override;
  bool PostNonNestableDelayedTask(const base::Location& from_here,
                                  base::OnceClosure task,
                                  base::TimeDelta delay) override;

 private:
  using TaskPair =
      std::tuple<base::Location, base::OnceClosure, base::TimeDelta>;
  ~BridgeTaskRunner() override;

  std::vector<TaskPair> tasks_;
  std::vector<TaskPair> non_nestable_tasks_;

  DISALLOW_COPY_AND_ASSIGN(BridgeTaskRunner);
};

}  // namespace atom

#endif  // ATOM_BROWSER_BRIDGE_TASK_RUNNER_H_
