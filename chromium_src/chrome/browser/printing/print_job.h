// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_PRINT_JOB_H_
#define CHROME_BROWSER_PRINTING_PRINT_JOB_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "chrome/browser/printing/print_job_worker_owner.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

#if defined(OS_WIN)
#include "printing/printed_page_win.h"
#endif

namespace base {
class RefCountedMemory;
}

namespace printing {

class JobEventDetails;
class MetafilePlayer;
class PdfToEmfConverter;
class PrintJobWorker;
class PrintedDocument;
#if defined(OS_WIN)
class PrintedPage;
#endif
class PrinterQuery;

// Manages the print work for a specific document. Talks to the printer through
// PrintingContext through PrintJobWorker. Hides access to PrintingContext in a
// worker thread so the caller never blocks. PrintJob will send notifications on
// any state change. While printing, the PrintJobManager instance keeps a
// reference to the job to be sure it is kept alive. All the code in this class
// runs in the UI thread.
class PrintJob : public PrintJobWorkerOwner,
                 public content::NotificationObserver {
 public:
  // Create a empty PrintJob. When initializing with this constructor,
  // post-constructor initialization must be done with Initialize().
  PrintJob();

  // Grabs the ownership of the PrintJobWorker from another job, which is
  // usually a PrinterQuery. Set the expected page count of the print job.
  void Initialize(PrintJobWorkerOwner* job,
                  const base::string16& name,
                  int page_count);

  // content::NotificationObserver implementation.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // PrintJobWorkerOwner implementation.
  void GetSettingsDone(const PrintSettings& new_settings,
                       PrintingContext::Result result) override;
  std::unique_ptr<PrintJobWorker> DetachWorker(
      PrintJobWorkerOwner* new_owner) override;
  const PrintSettings& settings() const override;
  int cookie() const override;

  // Starts the actual printing. Signals the worker that it should begin to
  // spool as soon as data is available.
  void StartPrinting();

  // Asks for the worker thread to finish its queued tasks and disconnects the
  // delegate object. The PrintJobManager will remove its reference. This may
  // have the side-effect of destroying the object if the caller doesn't have a
  // handle to the object. Use PrintJob::is_stopped() to check whether the
  // worker thread has actually stopped.
  void Stop();

  // Cancels printing job and stops the worker thread. Takes effect immediately.
  void Cancel();

  // Synchronously wait for the job to finish. It is mainly useful when the
  // process is about to be shut down and we're waiting for the spooler to eat
  // our data.
  bool FlushJob(base::TimeDelta timeout);

  // Returns true if the print job is pending, i.e. between a StartPrinting()
  // and the end of the spooling.
  bool is_job_pending() const;

  // Access the current printed document. Warning: may be NULL.
  PrintedDocument* document() const;

#if defined(OS_WIN)
  // Let the PrintJob know the 0-based |page_number| of a given printed page.
  void AppendPrintedPage(int page_number);

  void StartPdfToEmfConversion(
      const scoped_refptr<base::RefCountedMemory>& bytes,
      const gfx::Size& page_size,
      const gfx::Rect& content_area,
      bool print_text_with_gdi);

  void StartPdfToPostScriptConversion(
      const scoped_refptr<base::RefCountedMemory>& bytes,
      const gfx::Rect& content_area,
      const gfx::Point& physical_offset,
      bool ps_level2);
#endif  // defined(OS_WIN)

 protected:
  ~PrintJob() override;

 private:
  // Updates |document_| to a new instance.
  void UpdatePrintedDocument(PrintedDocument* new_document);

  // Processes a NOTIFY_PRINT_JOB_EVENT notification.
  void OnNotifyPrintJobEvent(const JobEventDetails& event_details);

  // Releases the worker thread by calling Stop(), then broadcasts a JOB_DONE
  // notification.
  void OnDocumentDone();

  // Terminates the worker thread in a very controlled way, to work around any
  // eventual deadlock.
  void ControlledWorkerShutdown();

  // Called at shutdown when running a nested message loop.
  void Quit();

  void HoldUntilStopIsCalled();

#if defined(OS_WIN)
  void OnPdfConversionStarted(int page_count);
  void OnPdfPageConverted(int page_number,
                          float scale_factor,
                          std::unique_ptr<MetafilePlayer> emf);
#endif  // defined(OS_WIN)

  content::NotificationRegistrar registrar_;

  // All the UI is done in a worker thread because many Win32 print functions
  // are blocking and enters a message loop without your consent. There is one
  // worker thread per print job.
  std::unique_ptr<PrintJobWorker> worker_;

  // Cache of the print context settings for access in the UI thread.
  PrintSettings settings_;

  // The printed document.
  scoped_refptr<PrintedDocument> document_;

  // Is the worker thread printing.
  bool is_job_pending_;

  // Is Canceling? If so, try to not cause recursion if on FAILED notification,
  // the notified calls Cancel() again.
  bool is_canceling_;

#if defined(OS_WIN)
  class PdfConversionState;
  std::unique_ptr<PdfConversionState> pdf_conversion_state_;
  std::vector<int> pdf_page_mapping_;
#endif  // defined(OS_WIN)

  // Used at shutdown so that we can quit a nested message loop.
  base::WeakPtrFactory<PrintJob> quit_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrintJob);
};

// Details for a NOTIFY_PRINT_JOB_EVENT notification. The members may be NULL.
class JobEventDetails : public base::RefCountedThreadSafe<JobEventDetails> {
 public:
  // Event type.
  enum Type {
    // Print... dialog box has been closed with OK button.
    USER_INIT_DONE,

    // Print... dialog box has been closed with CANCEL button.
    USER_INIT_CANCELED,

    // An automated initialization has been done, e.g. Init(false, NULL).
    DEFAULT_INIT_DONE,

    // A new document started printing.
    NEW_DOC,

    // A new page started printing.
    NEW_PAGE,

    // A page is done printing.
    PAGE_DONE,

    // A document is done printing. The worker thread is still alive. Warning:
    // not a good moment to release the handle to PrintJob.
    DOC_DONE,

    // The worker thread is finished. A good moment to release the handle to
    // PrintJob.
    JOB_DONE,

    // All missing pages have been requested.
    ALL_PAGES_REQUESTED,

    // An error occured. Printing is canceled.
    FAILED,
  };

#if defined(OS_WIN)
  JobEventDetails(Type type,
                  int job_id,
                  PrintedDocument* document,
                  PrintedPage* page);
#endif
  JobEventDetails(Type type, int job_id, PrintedDocument* document);

  // Getters.
  PrintedDocument* document() const;
#if defined(OS_WIN)
  PrintedPage* page() const;
#endif
  Type type() const { return type_; }
  int job_id() const { return job_id_; }

 private:
  friend class base::RefCountedThreadSafe<JobEventDetails>;

  ~JobEventDetails();

  scoped_refptr<PrintedDocument> document_;
#if defined(OS_WIN)
  scoped_refptr<PrintedPage> page_;
#endif
  const Type type_;
  int job_id_;

  DISALLOW_COPY_AND_ASSIGN(JobEventDetails);
};

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_PRINT_JOB_H_
