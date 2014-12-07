// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_PRINT_JOB_MANAGER_H_
#define CHROME_BROWSER_PRINTING_PRINT_JOB_MANAGER_H_

#include <set>
#include <vector>

#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "base/threading/non_thread_safe.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace printing {

class JobEventDetails;
class PrintJob;
class PrinterQuery;

class PrintQueriesQueue : public base::RefCountedThreadSafe<PrintQueriesQueue> {
 public:
  PrintQueriesQueue();

  // Queues a semi-initialized worker thread. Can be called from any thread.
  // Current use case is queuing from the I/O thread.
  // TODO(maruel):  Have them vanish after a timeout (~5 minutes?)
  void QueuePrinterQuery(PrinterQuery* job);

  // Pops a queued PrintJobWorkerOwner object that was previously queued or
  // create new one. Can be called from any thread.
  scoped_refptr<PrinterQuery> PopPrinterQuery(int document_cookie);

  // Creates new query.
  scoped_refptr<PrinterQuery> CreatePrinterQuery(int render_process_id,
                                                 int render_view_id);

  void Shutdown();

 private:
  friend class base::RefCountedThreadSafe<PrintQueriesQueue>;
  typedef std::vector<scoped_refptr<PrinterQuery> > PrinterQueries;

  virtual ~PrintQueriesQueue();

  // Used to serialize access to queued_workers_.
  base::Lock lock_;

  PrinterQueries queued_queries_;

  DISALLOW_COPY_AND_ASSIGN(PrintQueriesQueue);
};

class PrintJobManager : public content::NotificationObserver {
 public:
  PrintJobManager();
  virtual ~PrintJobManager();

  // On browser quit, we should wait to have the print job finished.
  void Shutdown();

  // content::NotificationObserver
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Returns queries queue. Never returns NULL. Must be called on Browser UI
  // Thread. Reference could be stored and used from any thread.
  scoped_refptr<PrintQueriesQueue> queue();

 private:
  typedef std::set<scoped_refptr<PrintJob> > PrintJobs;

  // Processes a NOTIFY_PRINT_JOB_EVENT notification.
  void OnPrintJobEvent(PrintJob* print_job,
                       const JobEventDetails& event_details);

  // Stops all printing jobs. If wait_for_finish is true, tries to give jobs
  // a chance to complete before stopping them.
  void StopJobs(bool wait_for_finish);

  content::NotificationRegistrar registrar_;

  // Current print jobs that are active.
  PrintJobs current_jobs_;

  scoped_refptr<PrintQueriesQueue> queue_;

  bool is_shutdown_;

  DISALLOW_COPY_AND_ASSIGN(PrintJobManager);
};

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_PRINT_JOB_MANAGER_H_
