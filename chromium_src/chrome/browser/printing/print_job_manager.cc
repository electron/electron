// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_job_manager.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/printing/print_job.h"
#include "chrome/browser/printing/printer_query.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "printing/printed_document.h"
#include "printing/printed_page.h"

namespace printing {

PrintQueriesQueue::PrintQueriesQueue() {
}

PrintQueriesQueue::~PrintQueriesQueue() {
  base::AutoLock lock(lock_);
  queued_queries_.clear();
}

void PrintQueriesQueue::SetDestination(PrintDestinationInterface* destination) {
  base::AutoLock lock(lock_);
  destination_ = destination;
}

void PrintQueriesQueue::QueuePrinterQuery(PrinterQuery* job) {
  base::AutoLock lock(lock_);
  DCHECK(job);
  queued_queries_.push_back(make_scoped_refptr(job));
  DCHECK(job->is_valid());
}

scoped_refptr<PrinterQuery> PrintQueriesQueue::PopPrinterQuery(
    int document_cookie) {
  base::AutoLock lock(lock_);
  for (PrinterQueries::iterator itr = queued_queries_.begin();
       itr != queued_queries_.end(); ++itr) {
    if ((*itr)->cookie() == document_cookie && !(*itr)->is_callback_pending()) {
      scoped_refptr<printing::PrinterQuery> current_query(*itr);
      queued_queries_.erase(itr);
      DCHECK(current_query->is_valid());
      return current_query;
    }
  }
  return NULL;
}

scoped_refptr<PrinterQuery> PrintQueriesQueue::CreatePrinterQuery() {
  scoped_refptr<PrinterQuery> job = new printing::PrinterQuery;
  base::AutoLock lock(lock_);
  job->SetWorkerDestination(destination_);
  return job;
}

void PrintQueriesQueue::Shutdown() {
  base::AutoLock lock(lock_);
  queued_queries_.clear();
  destination_ = NULL;
}

PrintJobManager::PrintJobManager() : is_shutdown_(false) {
  registrar_.Add(this, chrome::NOTIFICATION_PRINT_JOB_EVENT,
                 content::NotificationService::AllSources());
}

PrintJobManager::~PrintJobManager() {
}

scoped_refptr<PrintQueriesQueue> PrintJobManager::queue() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  if (!queue_)
    queue_ = new PrintQueriesQueue();
  return queue_;
}

void PrintJobManager::Shutdown() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(!is_shutdown_);
  is_shutdown_ = true;
  registrar_.RemoveAll();
  StopJobs(true);
  if (queue_)
    queue_->Shutdown();
  queue_ = NULL;
}

void PrintJobManager::StopJobs(bool wait_for_finish) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  // Copy the array since it can be modified in transit.
  PrintJobs to_stop;
  to_stop.swap(current_jobs_);

  for (PrintJobs::const_iterator job = to_stop.begin(); job != to_stop.end();
       ++job) {
    // Wait for two minutes for the print job to be spooled.
    if (wait_for_finish)
      (*job)->FlushJob(base::TimeDelta::FromMinutes(2));
    (*job)->Stop();
  }
}

void PrintJobManager::Observe(int type,
                              const content::NotificationSource& source,
                              const content::NotificationDetails& details) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  switch (type) {
    case chrome::NOTIFICATION_PRINT_JOB_EVENT: {
      OnPrintJobEvent(content::Source<PrintJob>(source).ptr(),
                      *content::Details<JobEventDetails>(details).ptr());
      break;
    }
    default: {
      NOTREACHED();
      break;
    }
  }
}

void PrintJobManager::OnPrintJobEvent(
    PrintJob* print_job,
    const JobEventDetails& event_details) {
  switch (event_details.type()) {
    case JobEventDetails::NEW_DOC: {
      DCHECK(current_jobs_.end() == current_jobs_.find(print_job));
      // Causes a AddRef().
      current_jobs_.insert(print_job);
      break;
    }
    case JobEventDetails::JOB_DONE: {
      DCHECK(current_jobs_.end() != current_jobs_.find(print_job));
      current_jobs_.erase(print_job);
      break;
    }
    case JobEventDetails::FAILED: {
      current_jobs_.erase(print_job);
      break;
    }
    case JobEventDetails::USER_INIT_DONE:
    case JobEventDetails::USER_INIT_CANCELED:
    case JobEventDetails::DEFAULT_INIT_DONE:
    case JobEventDetails::NEW_PAGE:
    case JobEventDetails::PAGE_DONE:
    case JobEventDetails::DOC_DONE:
    case JobEventDetails::ALL_PAGES_REQUESTED: {
      // Don't care.
      break;
    }
    default: {
      NOTREACHED();
      break;
    }
  }
}

}  // namespace printing
