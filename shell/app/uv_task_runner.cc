// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <algorithm>
#include <utility>

#include "base/location.h"
#include "base/time/time.h"
#include "shell/app/uv_task_runner.h"

namespace {

template <typename T>
void CloseHeapHandle(T* handle) {
  uv_close(reinterpret_cast<uv_handle_t*>(handle),
           [](uv_handle_t* closed) { delete reinterpret_cast<T*>(closed); });
}

}  // namespace

namespace electron {

UvTaskRunner::UvTaskRunner(uv_loop_t* loop) : loop_(loop) {}

UvTaskRunner::~UvTaskRunner() {
  std::ranges::for_each(tasks_, [](auto& it) { CloseHeapHandle(it.first); });
}

bool UvTaskRunner::PostDelayedTask(const base::Location& from_here,
                                   base::OnceClosure task,
                                   base::TimeDelta delay) {
  auto* timer = new uv_timer_t;
  timer->data = this;
  uv_timer_init(loop_, timer);
  uv_timer_start(timer, UvTaskRunner::OnTimeout, delay.InMilliseconds(), 0);
  tasks_.insert_or_assign(timer, std::move(task));
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
  if (auto item = tasks.extract(timer)) {
    std::move(item.mapped()).Run();
    CloseHeapHandle(timer);
  }
}

}  // namespace electron
