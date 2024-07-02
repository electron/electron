// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "base/location.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "shell/app/uv_task_runner.h"

namespace electron {

UvTaskRunner::UvTaskRunner(uv_loop_t* loop) : loop_(loop) {}

UvTaskRunner::~UvTaskRunner() = default;

bool UvTaskRunner::PostDelayedTask(const base::Location& from_here,
                                   base::OnceClosure task,
                                   base::TimeDelta delay) {
  auto* timer = new uv_timer_t;
  timer->data = this;
  uv_timer_init(loop_, timer);
  uv_timer_start(timer, UvTaskRunner::OnTimeout, delay.InMilliseconds(), 0);
  tasks_.try_emplace(UvHandle<uv_timer_t>{timer}, std::move(task));
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

// static
void UvTaskRunner::OnTimeout(uv_timer_t* timer) {
  auto& tasks = static_cast<UvTaskRunner*>(timer->data)->tasks_;

  for (auto it = std::begin(tasks), end = std::end(tasks); it != end; ++it) {
    if (it->first.get() == timer) {
      std::move(it->second).Run();
      tasks.erase(it);
      break;
    }
  }
}

}  // namespace electron
