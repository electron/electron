// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_view_manager_base.h"

#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "components/prefs/pref_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/timer/timer.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/printing/print_job.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/simple_message_box.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "printing/pdf_metafile_skia.h"
#include "printing/printed_document.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(ENABLE_FULL_PRINTING)
#include "chrome/browser/printing/print_error_dialog.h"
#endif

using base::TimeDelta;
using content::BrowserThread;

namespace printing {

namespace {

}  // namespace

PrintViewManagerBase::PrintViewManagerBase(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      number_pages_(0),
      printing_succeeded_(false),
      inside_inner_message_loop_(false),
      cookie_(0),
      queue_(g_browser_process->print_job_manager()->queue()) {
  DCHECK(queue_.get());
#if !defined(OS_MACOSX)
  expecting_first_page_ = true;
#endif  // OS_MACOSX
  printing_enabled_ = true;
}

PrintViewManagerBase::~PrintViewManagerBase() {
  ReleasePrinterQuery();
  DisconnectFromCurrentPrintJob();
}

#if !defined(DISABLE_BASIC_PRINTING)
bool PrintViewManagerBase::PrintNow(bool silent, bool print_background) {
  return PrintNowInternal(new PrintMsg_PrintPages(
      routing_id(), silent, print_background));
}
#endif  // !DISABLE_BASIC_PRINTING

void PrintViewManagerBase::NavigationStopped() {
  // Cancel the current job, wait for the worker to finish.
  TerminatePrintJob(true);
}

void PrintViewManagerBase::RenderProcessGone(base::TerminationStatus status) {
  ReleasePrinterQuery();

  if (!print_job_.get())
    return;

  scoped_refptr<PrintedDocument> document(print_job_->document());
  if (document.get()) {
    // If IsComplete() returns false, the document isn't completely rendered.
    // Since our renderer is gone, there's nothing to do, cancel it. Otherwise,
    // the print job may finish without problem.
    TerminatePrintJob(!document->IsComplete());
  }
}

base::string16 PrintViewManagerBase::RenderSourceName() {
  base::string16 name(web_contents()->GetTitle());
  return name;
}

void PrintViewManagerBase::OnDidGetPrintedPagesCount(int cookie,
                                                     int number_pages) {
  DCHECK_GT(cookie, 0);
  DCHECK_GT(number_pages, 0);
  number_pages_ = number_pages;
  OpportunisticallyCreatePrintJob(cookie);
}

void PrintViewManagerBase::OnDidGetDocumentCookie(int cookie) {
  cookie_ = cookie;
}

void PrintViewManagerBase::OnDidPrintPage(
  const PrintHostMsg_DidPrintPage_Params& params) {
  if (!OpportunisticallyCreatePrintJob(params.document_cookie))
    return;

  PrintedDocument* document = print_job_->document();
  if (!document || params.document_cookie != document->cookie()) {
    // Out of sync. It may happen since we are completely asynchronous. Old
    // spurious messages can be received if one of the processes is overloaded.
    return;
  }

#if defined(OS_MACOSX)
  const bool metafile_must_be_valid = true;
#else
  const bool metafile_must_be_valid = expecting_first_page_;
  expecting_first_page_ = false;
#endif  // OS_MACOSX

  base::SharedMemory shared_buf(params.metafile_data_handle, true);
  if (metafile_must_be_valid) {
    if (!shared_buf.Map(params.data_size)) {
      NOTREACHED() << "couldn't map";
      web_contents()->Stop();
      return;
    }
  }

  std::unique_ptr<PdfMetafileSkia> metafile(new PdfMetafileSkia);
  if (metafile_must_be_valid) {
    if (!metafile->InitFromData(shared_buf.memory(), params.data_size)) {
      NOTREACHED() << "Invalid metafile header";
      web_contents()->Stop();
      return;
    }
  }

#if !defined(OS_WIN)
  // Update the rendered document. It will send notifications to the listener.
  document->SetPage(params.page_number,
                    std::move(metafile),
                    params.page_size,
                    params.content_area);

  ShouldQuitFromInnerMessageLoop();
#else
  if (metafile_must_be_valid) {
    scoped_refptr<base::RefCountedBytes> bytes = new base::RefCountedBytes(
        reinterpret_cast<const unsigned char*>(shared_buf.memory()),
        params.data_size);

    document->DebugDumpData(bytes.get(), FILE_PATH_LITERAL(".pdf"));
    print_job_->StartPdfToEmfConversion(
        bytes, params.page_size, params.content_area);
  }
#endif  // !OS_WIN
}

void PrintViewManagerBase::OnPrintingFailed(int cookie) {
  if (cookie != cookie_) {
    NOTREACHED();
    return;
  }

  ReleasePrinterQuery();

  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PRINT_JOB_RELEASED,
      content::Source<content::WebContents>(web_contents()),
      content::NotificationService::NoDetails());
}

void PrintViewManagerBase::OnShowInvalidPrinterSettingsError() {
  LOG(ERROR) << "Invalid printer settings";
}

bool PrintViewManagerBase::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintViewManagerBase, message)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidGetPrintedPagesCount,
                        OnDidGetPrintedPagesCount)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidGetDocumentCookie,
                        OnDidGetDocumentCookie)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidPrintPage, OnDidPrintPage)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintingFailed, OnPrintingFailed)
    IPC_MESSAGE_HANDLER(PrintHostMsg_ShowInvalidPrinterSettingsError,
                        OnShowInvalidPrinterSettingsError);
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PrintViewManagerBase::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_PRINT_JOB_EVENT: {
      OnNotifyPrintJobEvent(*content::Details<JobEventDetails>(details).ptr());
      break;
    }
    default: {
      NOTREACHED();
      break;
    }
  }
}

void PrintViewManagerBase::OnNotifyPrintJobEvent(
    const JobEventDetails& event_details) {
  switch (event_details.type()) {
    case JobEventDetails::FAILED: {
      TerminatePrintJob(true);

      content::NotificationService::current()->Notify(
          chrome::NOTIFICATION_PRINT_JOB_RELEASED,
          content::Source<content::WebContents>(web_contents()),
          content::NotificationService::NoDetails());
      break;
    }
    case JobEventDetails::USER_INIT_DONE:
    case JobEventDetails::DEFAULT_INIT_DONE:
    case JobEventDetails::USER_INIT_CANCELED: {
      NOTREACHED();
      break;
    }
    case JobEventDetails::ALL_PAGES_REQUESTED: {
      ShouldQuitFromInnerMessageLoop();
      break;
    }
    case JobEventDetails::NEW_DOC:
    case JobEventDetails::NEW_PAGE:
    case JobEventDetails::PAGE_DONE:
    case JobEventDetails::DOC_DONE: {
      // Don't care about the actual printing process.
      break;
    }
    case JobEventDetails::JOB_DONE: {
      // Printing is done, we don't need it anymore.
      // print_job_->is_job_pending() may still be true, depending on the order
      // of object registration.
      printing_succeeded_ = true;
      ReleasePrintJob();

      content::NotificationService::current()->Notify(
          chrome::NOTIFICATION_PRINT_JOB_RELEASED,
          content::Source<content::WebContents>(web_contents()),
          content::NotificationService::NoDetails());
      break;
    }
    default: {
      NOTREACHED();
      break;
    }
  }
}

bool PrintViewManagerBase::RenderAllMissingPagesNow() {
  if (!print_job_.get() || !print_job_->is_job_pending())
    return false;

  // We can't print if there is no renderer.
  if (!web_contents() ||
      !web_contents()->GetRenderViewHost() ||
      !web_contents()->GetRenderViewHost()->IsRenderViewLive()) {
    return false;
  }

  // Is the document already complete?
  if (print_job_->document() && print_job_->document()->IsComplete()) {
    printing_succeeded_ = true;
    return true;
  }

  // WebContents is either dying or a second consecutive request to print
  // happened before the first had time to finish. We need to render all the
  // pages in an hurry if a print_job_ is still pending. No need to wait for it
  // to actually spool the pages, only to have the renderer generate them. Run
  // a message loop until we get our signal that the print job is satisfied.
  // PrintJob will send a ALL_PAGES_REQUESTED after having received all the
  // pages it needs. MessageLoop::current()->Quit() will be called as soon as
  // print_job_->document()->IsComplete() is true on either ALL_PAGES_REQUESTED
  // or in DidPrintPage(). The check is done in
  // ShouldQuitFromInnerMessageLoop().
  // BLOCKS until all the pages are received. (Need to enable recursive task)
  if (!RunInnerMessageLoop()) {
    // This function is always called from DisconnectFromCurrentPrintJob() so we
    // know that the job will be stopped/canceled in any case.
    return false;
  }
  return true;
}

void PrintViewManagerBase::ShouldQuitFromInnerMessageLoop() {
  // Look at the reason.
  DCHECK(print_job_->document());
  if (print_job_->document() &&
      print_job_->document()->IsComplete() &&
      inside_inner_message_loop_) {
    // We are in a message loop created by RenderAllMissingPagesNow. Quit from
    // it.
    base::MessageLoop::current()->QuitWhenIdle();
    inside_inner_message_loop_ = false;
  }
}

bool PrintViewManagerBase::CreateNewPrintJob(PrintJobWorkerOwner* job) {
  DCHECK(!inside_inner_message_loop_);

  // Disconnect the current print_job_.
  DisconnectFromCurrentPrintJob();

  // We can't print if there is no renderer.
  if (!web_contents()->GetRenderViewHost() ||
      !web_contents()->GetRenderViewHost()->IsRenderViewLive()) {
    return false;
  }

  // Ask the renderer to generate the print preview, create the print preview
  // view and switch to it, initialize the printer and show the print dialog.
  DCHECK(!print_job_.get());
  DCHECK(job);
  if (!job)
    return false;

  print_job_ = new PrintJob();
  print_job_->Initialize(job, this, number_pages_);
  registrar_.Add(this, chrome::NOTIFICATION_PRINT_JOB_EVENT,
                 content::Source<PrintJob>(print_job_.get()));
  printing_succeeded_ = false;
  return true;
}

void PrintViewManagerBase::DisconnectFromCurrentPrintJob() {
  // Make sure all the necessary rendered page are done. Don't bother with the
  // return value.
  bool result = RenderAllMissingPagesNow();

  // Verify that assertion.
  if (print_job_.get() &&
      print_job_->document() &&
      !print_job_->document()->IsComplete()) {
    DCHECK(!result);
    // That failed.
    TerminatePrintJob(true);
  } else {
    // DO NOT wait for the job to finish.
    ReleasePrintJob();
  }
#if !defined(OS_MACOSX)
  expecting_first_page_ = true;
#endif  // OS_MACOSX
}

void PrintViewManagerBase::PrintingDone(bool success) {
  if (!print_job_.get())
    return;
  Send(new PrintMsg_PrintingDone(routing_id(), success));
}

void PrintViewManagerBase::TerminatePrintJob(bool cancel) {
  if (!print_job_.get())
    return;

  if (cancel) {
    // We don't need the metafile data anymore because the printing is canceled.
    print_job_->Cancel();
    inside_inner_message_loop_ = false;
  } else {
    DCHECK(!inside_inner_message_loop_);
    DCHECK(!print_job_->document() || print_job_->document()->IsComplete());

    // WebContents is either dying or navigating elsewhere. We need to render
    // all the pages in an hurry if a print job is still pending. This does the
    // trick since it runs a blocking message loop:
    print_job_->Stop();
  }
  ReleasePrintJob();
}

void PrintViewManagerBase::ReleasePrintJob() {
  if (!print_job_.get())
    return;

  PrintingDone(printing_succeeded_);

  registrar_.Remove(this, chrome::NOTIFICATION_PRINT_JOB_EVENT,
                    content::Source<PrintJob>(print_job_.get()));
  print_job_->DisconnectSource();
  // Don't close the worker thread.
  print_job_ = NULL;
}

bool PrintViewManagerBase::RunInnerMessageLoop() {
  // This value may actually be too low:
  //
  // - If we're looping because of printer settings initialization, the premise
  // here is that some poor users have their print server away on a VPN over a
  // slow connection. In this situation, the simple fact of opening the printer
  // can be dead slow. On the other side, we don't want to die infinitely for a
  // real network error. Give the printer 60 seconds to comply.
  //
  // - If we're looping because of renderer page generation, the renderer could
  // be CPU bound, the page overly complex/large or the system just
  // memory-bound.
  static const int kPrinterSettingsTimeout = 60000;
  base::OneShotTimer quit_timer;
  quit_timer.Start(
      FROM_HERE, TimeDelta::FromMilliseconds(kPrinterSettingsTimeout),
      base::MessageLoop::current(), &base::MessageLoop::QuitWhenIdle);

  inside_inner_message_loop_ = true;

  // Need to enable recursive task.
  {
    base::MessageLoop::ScopedNestableTaskAllower allow(
        base::MessageLoop::current());
    base::MessageLoop::current()->Run();
  }

  bool success = true;
  if (inside_inner_message_loop_) {
    // Ok we timed out. That's sad.
    inside_inner_message_loop_ = false;
    success = false;
  }

  return success;
}

bool PrintViewManagerBase::OpportunisticallyCreatePrintJob(int cookie) {
  if (print_job_.get())
    return true;

  if (!cookie) {
    // Out of sync. It may happens since we are completely asynchronous. Old
    // spurious message can happen if one of the processes is overloaded.
    return false;
  }

  // The job was initiated by a script. Time to get the corresponding worker
  // thread.
  scoped_refptr<PrinterQuery> queued_query = queue_->PopPrinterQuery(cookie);
  if (!queued_query.get()) {
    NOTREACHED();
    return false;
  }

  if (!CreateNewPrintJob(queued_query.get())) {
    // Don't kill anything.
    return false;
  }

  // Settings are already loaded. Go ahead. This will set
  // print_job_->is_job_pending() to true.
  print_job_->StartPrinting();
  return true;
}

bool PrintViewManagerBase::PrintNowInternal(IPC::Message* message) {
  // Don't print / print preview interstitials.
  if (web_contents()->ShowingInterstitialPage()) {
    delete message;
    return false;
  }
  return Send(message);
}

void PrintViewManagerBase::ReleasePrinterQuery() {
  if (!cookie_)
    return;

  int cookie = cookie_;
  cookie_ = 0;

  printing::PrintJobManager* print_job_manager =
      g_browser_process->print_job_manager();
  // May be NULL in tests.
  if (!print_job_manager)
    return;

  scoped_refptr<printing::PrinterQuery> printer_query;
  printer_query = queue_->PopPrinterQuery(cookie);
  if (!printer_query.get())
    return;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&PrinterQuery::StopWorker, printer_query.get()));
}

}  // namespace printing
