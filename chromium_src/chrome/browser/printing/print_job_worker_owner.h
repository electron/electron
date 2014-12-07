// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_PRINT_JOB_WORKER_OWNER_H__
#define CHROME_BROWSER_PRINTING_PRINT_JOB_WORKER_OWNER_H__

#include "base/memory/ref_counted.h"
#include "printing/printing_context.h"

namespace base {
class MessageLoop;
class SequencedTaskRunner;
}

namespace tracked_objects {
class Location;
}

namespace printing {

class PrintJobWorker;
class PrintSettings;

class PrintJobWorkerOwner
    : public base::RefCountedThreadSafe<PrintJobWorkerOwner> {
 public:
  PrintJobWorkerOwner();

  // Finishes the initialization began by PrintJobWorker::GetSettings().
  // Creates a new PrintedDocument if necessary. Solely meant to be called by
  // PrintJobWorker.
  virtual void GetSettingsDone(const PrintSettings& new_settings,
                               PrintingContext::Result result) = 0;

  // Detach the PrintJobWorker associated to this object.
  virtual PrintJobWorker* DetachWorker(PrintJobWorkerOwner* new_owner) = 0;

  // Access the current settings.
  virtual const PrintSettings& settings() const = 0;

  // Cookie uniquely identifying the PrintedDocument and/or loaded settings.
  virtual int cookie() const = 0;

  // Returns true if the current thread is a thread on which a task
  // may be run, and false if no task will be run on the current
  // thread.
  bool RunsTasksOnCurrentThread() const;

  // Posts the given task to be run.
  bool PostTask(const tracked_objects::Location& from_here,
                const base::Closure& task);

 protected:
  friend class base::RefCountedThreadSafe<PrintJobWorkerOwner>;

  virtual ~PrintJobWorkerOwner();

  // Task runner reference. Used to send notifications in the right
  // thread.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_PRINT_JOB_WORKER_OWNER_H__
