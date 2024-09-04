// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/uv_task_runner.h"

#include <utility>

#include "base/location.h"
#include "base/time/time.h"

namespace electron {

UvTaskRunner::UvTaskRunner(uv_loop_t* loop) : loop_{loop} {}

UvTaskRunner::~UvTaskRunner() = default;

bool UvTaskRunner::PostDelayedTask(const base::Location& from_here,
                                   base::OnceClosure task,
                                   base::TimeDelta delay) {
  auto on_timeout = [](uv_timer_t* timer) {
    auto& tasks = static_cast<UvTaskRunner*>(timer->data)->tasks_;
    if (auto iter = tasks.find(timer); iter != tasks.end())
      std::move(tasks.extract(iter).mapped()).Run();
  };

  auto timer = UvHandle<uv_timer_t>{};
  timer->data = this;
  uv_timer_init(loop_, timer.get());
  uv_timer_start(timer.get(), on_timeout, delay.InMilliseconds(), 0);
  tasks_.insert_or_assign(std::move(timer), std::move(task));
  return true;
}

bool UvTaskRunner::RunsTasksInCurrentSequence() const {
  return true;
}

bool UvTaskRunner::PostNonNestableDelayedTask(const base::Location& from_here,
                                              base::OnceClosure task,
                                              base::TimeDelta delay) {
  return PostDelayedTask(from_here, std::move(task), delay);
}

}  // namespace electron
