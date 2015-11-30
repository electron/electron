// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_job.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/worker_pool.h"
#include "base/timer/timer.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/printing/print_job_worker.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "printing/printed_document.h"
#include "printing/printed_page.h"

#if defined(OS_WIN)
#include "chrome/browser/printing/pdf_to_emf_converter.h"
#include "printing/pdf_render_settings.h"
#endif


using base::TimeDelta;

namespace {

// Helper function to ensure |owner| is valid until at least |callback| returns.
void HoldRefCallback(const scoped_refptr<printing::PrintJobWorkerOwner>& owner,
                     const base::Closure& callback) {
  callback.Run();
}

}  // namespace

namespace printing {

PrintJob::PrintJob()
    : source_(NULL),
      worker_(),
      settings_(),
      is_job_pending_(false),
      is_canceling_(false),
      quit_factory_(this) {
  // This is normally a UI message loop, but in unit tests, the message loop is
  // of the 'default' type.
  DCHECK(base::MessageLoopForUI::IsCurrent() ||
         base::MessageLoop::current()->type() ==
             base::MessageLoop::TYPE_DEFAULT);
}

PrintJob::~PrintJob() {
  // The job should be finished (or at least canceled) when it is destroyed.
  DCHECK(!is_job_pending_);
  DCHECK(!is_canceling_);
  DCHECK(!worker_ || !worker_->IsRunning());
  DCHECK(RunsTasksOnCurrentThread());
}

void PrintJob::Initialize(PrintJobWorkerOwner* job,
                          PrintedPagesSource* source,
                          int page_count) {
  DCHECK(!source_);
  DCHECK(!worker_.get());
  DCHECK(!is_job_pending_);
  DCHECK(!is_canceling_);
  DCHECK(!document_.get());
  source_ = source;
  worker_.reset(job->DetachWorker(this));
  settings_ = job->settings();

  PrintedDocument* new_doc =
      new PrintedDocument(settings_,
                          source_,
                          job->cookie(),
                          content::BrowserThread::GetBlockingPool());
  new_doc->set_page_count(page_count);
  UpdatePrintedDocument(new_doc);

  // Don't forget to register to our own messages.
  registrar_.Add(this, chrome::NOTIFICATION_PRINT_JOB_EVENT,
                 content::Source<PrintJob>(this));
}

void PrintJob::Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) {
  DCHECK(RunsTasksOnCurrentThread());
  switch (type) {
    case chrome::NOTIFICATION_PRINT_JOB_EVENT: {
      OnNotifyPrintJobEvent(*content::Details<JobEventDetails>(details).ptr());
      break;
    }
    default: {
      break;
    }
  }
}

void PrintJob::GetSettingsDone(const PrintSettings& new_settings,
                               PrintingContext::Result result) {
  NOTREACHED();
}

PrintJobWorker* PrintJob::DetachWorker(PrintJobWorkerOwner* new_owner) {
  NOTREACHED();
  return NULL;
}

const PrintSettings& PrintJob::settings() const {
  return settings_;
}

int PrintJob::cookie() const {
  if (!document_.get())
    // Always use an invalid cookie in this case.
    return 0;
  return document_->cookie();
}

void PrintJob::StartPrinting() {
  DCHECK(RunsTasksOnCurrentThread());
  DCHECK(worker_->IsRunning());
  DCHECK(!is_job_pending_);
  if (!worker_->IsRunning() || is_job_pending_)
    return;

  // Real work is done in PrintJobWorker::StartPrinting().
  worker_->PostTask(FROM_HERE,
                    base::Bind(&HoldRefCallback,
                               make_scoped_refptr(this),
                               base::Bind(&PrintJobWorker::StartPrinting,
                                          base::Unretained(worker_.get()),
                                          document_)));
  // Set the flag right now.
  is_job_pending_ = true;

  // Tell everyone!
  scoped_refptr<JobEventDetails> details(
      new JobEventDetails(JobEventDetails::NEW_DOC, document_.get(), NULL));
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PRINT_JOB_EVENT,
      content::Source<PrintJob>(this),
      content::Details<JobEventDetails>(details.get()));
}

void PrintJob::Stop() {
  DCHECK(RunsTasksOnCurrentThread());

  if (quit_factory_.HasWeakPtrs()) {
    // In case we're running a nested message loop to wait for a job to finish,
    // and we finished before the timeout, quit the nested loop right away.
    Quit();
    quit_factory_.InvalidateWeakPtrs();
  }

  // Be sure to live long enough.
  scoped_refptr<PrintJob> handle(this);

  if (worker_->IsRunning()) {
    ControlledWorkerShutdown();
  } else {
    // Flush the cached document.
    UpdatePrintedDocument(NULL);
  }
}

void PrintJob::Cancel() {
  if (is_canceling_)
    return;
  is_canceling_ = true;

  // Be sure to live long enough.
  scoped_refptr<PrintJob> handle(this);

  DCHECK(RunsTasksOnCurrentThread());
  if (worker_ && worker_->IsRunning()) {
    // Call this right now so it renders the context invalid. Do not use
    // InvokeLater since it would take too much time.
    worker_->Cancel();
  }
  // Make sure a Cancel() is broadcast.
  scoped_refptr<JobEventDetails> details(
      new JobEventDetails(JobEventDetails::FAILED, NULL, NULL));
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PRINT_JOB_EVENT,
      content::Source<PrintJob>(this),
      content::Details<JobEventDetails>(details.get()));
  Stop();
  is_canceling_ = false;
}

bool PrintJob::FlushJob(base::TimeDelta timeout) {
  // Make sure the object outlive this message loop.
  scoped_refptr<PrintJob> handle(this);

  base::MessageLoop::current()->PostDelayedTask(FROM_HERE,
      base::Bind(&PrintJob::Quit, quit_factory_.GetWeakPtr()), timeout);

  base::MessageLoop::ScopedNestableTaskAllower allow(
      base::MessageLoop::current());
  base::MessageLoop::current()->Run();

  return true;
}

void PrintJob::DisconnectSource() {
  source_ = NULL;
  if (document_.get())
    document_->DisconnectSource();
}

bool PrintJob::is_job_pending() const {
  return is_job_pending_;
}

PrintedDocument* PrintJob::document() const {
  return document_.get();
}

#if defined(OS_WIN)

class PrintJob::PdfToEmfState {
 public:
  PdfToEmfState(const gfx::Size& page_size, const gfx::Rect& content_area)
      : page_count_(0),
        current_page_(0),
        pages_in_progress_(0),
        page_size_(page_size),
        content_area_(content_area),
        converter_(PdfToEmfConverter::CreateDefault()) {}

  void Start(const scoped_refptr<base::RefCountedMemory>& data,
             const PdfRenderSettings& conversion_settings,
             const PdfToEmfConverter::StartCallback& start_callback) {
    converter_->Start(data, conversion_settings, start_callback);
  }

  void GetMorePages(
      const PdfToEmfConverter::GetPageCallback& get_page_callback) {
    const int kMaxNumberOfTempFilesPerDocument = 3;
    while (pages_in_progress_ < kMaxNumberOfTempFilesPerDocument &&
           current_page_ < page_count_) {
      ++pages_in_progress_;
      converter_->GetPage(current_page_++, get_page_callback);
    }
  }

  void OnPageProcessed(
      const PdfToEmfConverter::GetPageCallback& get_page_callback) {
    --pages_in_progress_;
    GetMorePages(get_page_callback);
    // Release converter if we don't need this any more.
    if (!pages_in_progress_ && current_page_ >= page_count_)
      converter_.reset();
  }

  void set_page_count(int page_count) { page_count_ = page_count; }
  gfx::Size page_size() const { return page_size_; }
  gfx::Rect content_area() const { return content_area_; }

 private:
  int page_count_;
  int current_page_;
  int pages_in_progress_;
  gfx::Size page_size_;
  gfx::Rect content_area_;
  scoped_ptr<PdfToEmfConverter> converter_;
};

void PrintJob::StartPdfToEmfConversion(
    const scoped_refptr<base::RefCountedMemory>& bytes,
    const gfx::Size& page_size,
    const gfx::Rect& content_area) {
  DCHECK(!ptd_to_emf_state_.get());
  ptd_to_emf_state_.reset(new PdfToEmfState(page_size, content_area));
  const int kPrinterDpi = settings().dpi();
  ptd_to_emf_state_->Start(
      bytes,
      printing::PdfRenderSettings(content_area, kPrinterDpi, true),
      base::Bind(&PrintJob::OnPdfToEmfStarted, this));
}

void PrintJob::OnPdfToEmfStarted(int page_count) {
  if (page_count <= 0) {
    ptd_to_emf_state_.reset();
    Cancel();
    return;
  }
  ptd_to_emf_state_->set_page_count(page_count);
  ptd_to_emf_state_->GetMorePages(
      base::Bind(&PrintJob::OnPdfToEmfPageConverted, this));
}

void PrintJob::OnPdfToEmfPageConverted(int page_number,
                                       float scale_factor,
                                       scoped_ptr<MetafilePlayer> emf) {
  DCHECK(ptd_to_emf_state_);
  if (!document_.get() || !emf) {
    ptd_to_emf_state_.reset();
    Cancel();
    return;
  }

  // Update the rendered document. It will send notifications to the listener.
  document_->SetPage(page_number,
                     emf.Pass(),
                     scale_factor,
                     ptd_to_emf_state_->page_size(),
                     ptd_to_emf_state_->content_area());

  ptd_to_emf_state_->GetMorePages(
      base::Bind(&PrintJob::OnPdfToEmfPageConverted, this));
}

#endif  // OS_WIN

void PrintJob::UpdatePrintedDocument(PrintedDocument* new_document) {
  if (document_.get() == new_document)
    return;

  document_ = new_document;

  if (document_.get()) {
    settings_ = document_->settings();
  }

  if (worker_) {
    DCHECK(!is_job_pending_);
    // Sync the document with the worker.
    worker_->PostTask(FROM_HERE,
                      base::Bind(&HoldRefCallback,
                                 make_scoped_refptr(this),
                                 base::Bind(&PrintJobWorker::OnDocumentChanged,
                                            base::Unretained(worker_.get()),
                                            document_)));
  }
}

void PrintJob::OnNotifyPrintJobEvent(const JobEventDetails& event_details) {
  switch (event_details.type()) {
    case JobEventDetails::FAILED: {
      settings_.Clear();
      // No need to cancel since the worker already canceled itself.
      Stop();
      break;
    }
    case JobEventDetails::USER_INIT_DONE:
    case JobEventDetails::DEFAULT_INIT_DONE:
    case JobEventDetails::USER_INIT_CANCELED: {
      DCHECK_EQ(event_details.document(), document_.get());
      break;
    }
    case JobEventDetails::NEW_DOC:
    case JobEventDetails::NEW_PAGE:
    case JobEventDetails::JOB_DONE:
    case JobEventDetails::ALL_PAGES_REQUESTED: {
      // Don't care.
      break;
    }
    case JobEventDetails::DOC_DONE: {
      // This will call Stop() and broadcast a JOB_DONE message.
      base::MessageLoop::current()->PostTask(
          FROM_HERE, base::Bind(&PrintJob::OnDocumentDone, this));
      break;
    }
    case JobEventDetails::PAGE_DONE:
#if defined(OS_WIN)
      ptd_to_emf_state_->OnPageProcessed(
          base::Bind(&PrintJob::OnPdfToEmfPageConverted, this));
#endif  // OS_WIN
      break;
    default: {
      NOTREACHED();
      break;
    }
  }
}

void PrintJob::OnDocumentDone() {
  // Be sure to live long enough. The instance could be destroyed by the
  // JOB_DONE broadcast.
  scoped_refptr<PrintJob> handle(this);

  // Stop the worker thread.
  Stop();

  scoped_refptr<JobEventDetails> details(
      new JobEventDetails(JobEventDetails::JOB_DONE, document_.get(), NULL));
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PRINT_JOB_EVENT,
      content::Source<PrintJob>(this),
      content::Details<JobEventDetails>(details.get()));
}

void PrintJob::ControlledWorkerShutdown() {
  DCHECK(RunsTasksOnCurrentThread());

  // The deadlock this code works around is specific to window messaging on
  // Windows, so we aren't likely to need it on any other platforms.
#if defined(OS_WIN)
  // We could easily get into a deadlock case if worker_->Stop() is used; the
  // printer driver created a window as a child of the browser window. By
  // canceling the job, the printer driver initiated dialog box is destroyed,
  // which sends a blocking message to its parent window. If the browser window
  // thread is not processing messages, a deadlock occurs.
  //
  // This function ensures that the dialog box will be destroyed in a timely
  // manner by the mere fact that the thread will terminate. So the potential
  // deadlock is eliminated.
  worker_->StopSoon();

  // Delay shutdown until the worker terminates.  We want this code path
  // to wait on the thread to quit before continuing.
  if (worker_->IsRunning()) {
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&PrintJob::ControlledWorkerShutdown, this),
        base::TimeDelta::FromMilliseconds(100));
    return;
  }
#endif


  // Now make sure the thread object is cleaned up. Do this on a worker
  // thread because it may block.
  base::WorkerPool::PostTaskAndReply(
      FROM_HERE,
      base::Bind(&PrintJobWorker::Stop, base::Unretained(worker_.get())),
      base::Bind(&PrintJob::HoldUntilStopIsCalled, this),
      false);

  is_job_pending_ = false;
  registrar_.RemoveAll();
  UpdatePrintedDocument(NULL);
}

void PrintJob::HoldUntilStopIsCalled() {
}

void PrintJob::Quit() {
  base::MessageLoop::current()->Quit();
}

// Takes settings_ ownership and will be deleted in the receiving thread.
JobEventDetails::JobEventDetails(Type type,
                                 PrintedDocument* document,
                                 PrintedPage* page)
    : document_(document),
      page_(page),
      type_(type) {
}

JobEventDetails::~JobEventDetails() {
}

PrintedDocument* JobEventDetails::document() const { return document_.get(); }

PrintedPage* JobEventDetails::page() const { return page_.get(); }

}  // namespace printing
