// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/bridge_task_runner.h"

#include <utility>

#include "base/message_loop/message_loop.h"

namespace atom {

BridgeTaskRunner::BridgeTaskRunner() = default;
BridgeTaskRunner::~BridgeTaskRunner() = default;

void BridgeTaskRunner::MessageLoopIsReady() {
  auto message_loop = base::MessageLoop::current();
  CHECK(message_loop);
  for (TaskPair& task : tasks_) {
    message_loop->task_runner()->PostDelayedTask(
        std::get<0>(task), std::move(std::get<1>(task)), std::get<2>(task));
  }
  for (TaskPair& task : non_nestable_tasks_) {
    message_loop->task_runner()->PostNonNestableDelayedTask(
        std::get<0>(task), std::move(std::get<1>(task)), std::get<2>(task));
  }
}

bool BridgeTaskRunner::PostDelayedTask(const base::Location& from_here,
                                       base::OnceClosure task,
                                       base::TimeDelta delay) {
  auto message_loop = base::MessageLoop::current();
  if (!message_loop) {
    tasks_.push_back(std::make_tuple(from_here, std::move(task), delay));
    return true;
  }

  return message_loop->task_runner()->PostDelayedTask(from_here,
                                                      std::move(task), delay);
}

bool BridgeTaskRunner::RunsTasksInCurrentSequence() const {
  auto message_loop = base::MessageLoop::current();
  if (!message_loop)
    return true;

  return message_loop->task_runner()->RunsTasksInCurrentSequence();
}

bool BridgeTaskRunner::PostNonNestableDelayedTask(
    const base::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  auto message_loop = base::MessageLoop::current();
  if (!message_loop) {
    non_nestable_tasks_.push_back(
        std::make_tuple(from_here, std::move(task), delay));
    return true;
  }

  return message_loop->task_runner()->PostNonNestableDelayedTask(
      from_here, std::move(task), delay);
}

}  // namespace atom
