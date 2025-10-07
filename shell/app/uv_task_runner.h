// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_UV_TASK_RUNNER_H_
#define ELECTRON_SHELL_APP_UV_TASK_RUNNER_H_

#include <map>

#include "base/memory/raw_ptr.h"
#include "base/task/single_thread_task_runner.h"
#include "shell/common/node_bindings.h"

namespace base {
class Location;
class TimeDelta;
}  // namespace base

namespace electron {

// TaskRunner implementation that posts tasks into libuv's default loop.
class UvTaskRunner : public base::SingleThreadTaskRunner {
 public:
  explicit UvTaskRunner(uv_loop_t* loop);

  // disable copy
  UvTaskRunner(const UvTaskRunner&) = delete;
  UvTaskRunner& operator=(const UvTaskRunner&) = delete;

  // base::SingleThreadTaskRunner:
  bool PostDelayedTask(const base::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override;
  bool RunsTasksInCurrentSequence() const override;
  bool PostNonNestableDelayedTask(const base::Location& from_here,
                                  base::OnceClosure task,
                                  base::TimeDelta delay) override;

 private:
  ~UvTaskRunner() override;

  raw_ptr<uv_loop_t> loop_;

  std::map<UvHandle<uv_timer_t>, base::OnceClosure, UvHandleCompare> tasks_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_APP_UV_TASK_RUNNER_H_
