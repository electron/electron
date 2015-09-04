// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/bridge_task_runner.h"

#include "base/message_loop/message_loop.h"

namespace atom {

bool BridgeTaskRunner::PostDelayedTask(
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay) {
  auto message_loop = base::MessageLoop::current();
  if (!message_loop)
    return false;

  return message_loop->task_runner()->PostDelayedTask(from_here, task, delay);
}

bool BridgeTaskRunner::RunsTasksOnCurrentThread() const {
  auto message_loop = base::MessageLoop::current();
  if (!message_loop)
    return false;

  return message_loop->task_runner()->RunsTasksOnCurrentThread();
}

bool BridgeTaskRunner::PostNonNestableDelayedTask(
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay) {
  auto message_loop = base::MessageLoop::current();
  if (!message_loop)
    return false;

  return message_loop->task_runner()->PostNonNestableDelayedTask(
      from_here, task, delay);
}

}  // namespace atom
