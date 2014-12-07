// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_job_worker_owner.h"

#include "base/message_loop/message_loop.h"

namespace printing {

PrintJobWorkerOwner::PrintJobWorkerOwner()
    : task_runner_(base::MessageLoop::current()->task_runner()) {
}

PrintJobWorkerOwner::~PrintJobWorkerOwner() {
}

bool PrintJobWorkerOwner::RunsTasksOnCurrentThread() const {
  return task_runner_->RunsTasksOnCurrentThread();
}

bool PrintJobWorkerOwner::PostTask(const tracked_objects::Location& from_here,
                                   const base::Closure& task) {
  return task_runner_->PostTask(from_here, task);
}

}  // namespace printing
