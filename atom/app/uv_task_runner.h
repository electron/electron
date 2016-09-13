// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_UV_TASK_RUNNER_H_
#define ATOM_APP_UV_TASK_RUNNER_H_

#include <map>

#include "base/callback.h"
#include "base/single_thread_task_runner.h"
#include "vendor/node/deps/uv/include/uv.h"

namespace atom {

// TaskRunner implementation that posts tasks into libuv's default loop.
class UvTaskRunner : public base::SingleThreadTaskRunner {
 public:
  explicit UvTaskRunner(uv_loop_t* loop);
  ~UvTaskRunner() override;

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
  static void OnTimeout(uv_timer_t* timer);
  static void OnClose(uv_handle_t* handle);

  uv_loop_t* loop_;

  std::map<uv_timer_t*, base::Closure> tasks_;

  DISALLOW_COPY_AND_ASSIGN(UvTaskRunner);
};

}  // namespace atom

#endif  // ATOM_APP_UV_TASK_RUNNER_H_
