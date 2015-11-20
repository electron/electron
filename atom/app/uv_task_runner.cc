// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/uv_task_runner.h"

#include "base/stl_util.h"

namespace atom {

UvTaskRunner::UvTaskRunner(uv_loop_t* loop) : loop_(loop) {
}

UvTaskRunner::~UvTaskRunner() {
  for (auto& iter : tasks_) {
    uv_unref(reinterpret_cast<uv_handle_t*>(iter.first));
    delete iter.first;
  }
}

bool UvTaskRunner::PostDelayedTask(const tracked_objects::Location& from_here,
                                   const base::Closure& task,
                                   base::TimeDelta delay) {
  uv_timer_t* timer = new uv_timer_t;
  timer->data = this;
  uv_timer_init(loop_, timer);
  uv_timer_start(timer, UvTaskRunner::OnTimeout, delay.InMilliseconds(), 0);
  tasks_[timer] = task;
  return true;
}

bool UvTaskRunner::RunsTasksOnCurrentThread() const {
  return true;
}

bool UvTaskRunner::PostNonNestableDelayedTask(
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay) {
  return PostDelayedTask(from_here, task, delay);;
}

// static
void UvTaskRunner::OnTimeout(uv_timer_t* timer) {
  UvTaskRunner* self = static_cast<UvTaskRunner*>(timer->data);
  if (!ContainsKey(self->tasks_, timer))
    return;

  self->tasks_[timer].Run();
  self->tasks_.erase(timer);
  uv_timer_stop(timer);
  uv_close(reinterpret_cast<uv_handle_t*>(timer), UvTaskRunner::OnClose);
}

// static
void UvTaskRunner::OnClose(uv_handle_t* handle) {
  delete reinterpret_cast<uv_timer_t*>(handle);
}

}  // namespace atom
