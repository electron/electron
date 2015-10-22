// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/bridge_task_runner.h"

#include "base/message_loop/message_loop.h"

namespace atom {

// static
std::vector<BridgeTaskRunner::TaskPair> BridgeTaskRunner::tasks_;
std::vector<BridgeTaskRunner::TaskPair> BridgeTaskRunner::non_nestable_tasks_;

// static
void BridgeTaskRunner::MessageLoopIsReady() {
  auto message_loop = base::MessageLoop::current();
  CHECK(message_loop);
  for (const TaskPair& task : tasks_) {
    message_loop->task_runner()->PostDelayedTask(
        base::get<0>(task), base::get<1>(task), base::get<2>(task));
  }
  for (const TaskPair& task : non_nestable_tasks_) {
    message_loop->task_runner()->PostNonNestableDelayedTask(
        base::get<0>(task), base::get<1>(task), base::get<2>(task));
  }
}

bool BridgeTaskRunner::PostDelayedTask(
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay) {
  auto message_loop = base::MessageLoop::current();
  if (!message_loop) {
    tasks_.push_back(base::MakeTuple(from_here, task, delay));
    return true;
  }

  return message_loop->task_runner()->PostDelayedTask(from_here, task, delay);
}

bool BridgeTaskRunner::RunsTasksOnCurrentThread() const {
  auto message_loop = base::MessageLoop::current();
  if (!message_loop)
    return true;

  return message_loop->task_runner()->RunsTasksOnCurrentThread();
}

bool BridgeTaskRunner::PostNonNestableDelayedTask(
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay) {
  auto message_loop = base::MessageLoop::current();
  if (!message_loop) {
    non_nestable_tasks_.push_back(base::MakeTuple(from_here, task, delay));
    return true;
  }

  return message_loop->task_runner()->PostNonNestableDelayedTask(
      from_here, task, delay);
}

}  // namespace atom
