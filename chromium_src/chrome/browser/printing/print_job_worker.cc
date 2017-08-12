// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_job_worker.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/printing/print_job.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "printing/print_job_constants.h"
#include "printing/printed_document.h"
#include "printing/printed_page.h"
#include "printing/printing_utils.h"
#include "ui/base/l10n/l10n_util.h"

#include <stddef.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "printing/page_size_margins.h"
#include "printing/print_job_constants.h"
#include "printing/print_settings.h"
#include "printing/units.h"

using content::BrowserThread;

namespace printing {

namespace {

// Helper function to ensure |owner| is valid until at least |callback| returns.
void HoldRefCallback(const scoped_refptr<PrintJobWorkerOwner>& owner,
                     const base::Closure& callback) {
  callback.Run();
}

void SetCustomMarginsToJobSettings(const PageSizeMargins& page_size_margins,
                                   base::DictionaryValue* settings) {
  std::unique_ptr<base::DictionaryValue> custom_margins(new base::DictionaryValue());
  custom_margins->SetDouble(kSettingMarginTop, page_size_margins.margin_top);
  custom_margins->SetDouble(kSettingMarginBottom, page_size_margins.margin_bottom);
  custom_margins->SetDouble(kSettingMarginLeft, page_size_margins.margin_left);
  custom_margins->SetDouble(kSettingMarginRight, page_size_margins.margin_right);
  settings->Set(kSettingMarginsCustom, std::move(custom_margins));
}

void PrintSettingsToJobSettings(const PrintSettings& settings,
                                base::DictionaryValue* job_settings) {
  // header footer
  job_settings->SetBoolean(kSettingHeaderFooterEnabled,
                           settings.display_header_footer());
  job_settings->SetString(kSettingHeaderFooterTitle, settings.title());
  job_settings->SetString(kSettingHeaderFooterURL, settings.url());

  // bg
  job_settings->SetBoolean(kSettingShouldPrintBackgrounds,
                           settings.should_print_backgrounds());
  job_settings->SetBoolean(kSettingShouldPrintSelectionOnly,
                           settings.selection_only());

  // margin
  auto margin_type = settings.margin_type();
  job_settings->SetInteger(kSettingMarginsType, settings.margin_type());
  if (margin_type == CUSTOM_MARGINS) {
    const auto& margins_in_points = settings.requested_custom_margins_in_points();

    PageSizeMargins page_size_margins;

    page_size_margins.margin_top = margins_in_points.top;
    page_size_margins.margin_bottom = margins_in_points.bottom;
    page_size_margins.margin_left = margins_in_points.left;
    page_size_margins.margin_right = margins_in_points.right;
    SetCustomMarginsToJobSettings(page_size_margins, job_settings);
  }
  job_settings->SetInteger(kSettingPreviewPageCount, 1);

  // range

  if (!settings.ranges().empty()) {
    auto page_range_array = base::MakeUnique<base::ListValue>();
    job_settings->Set(kSettingPageRange, std::move(page_range_array));
    for (size_t i = 0; i < settings.ranges().size(); ++i) {
      std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
      dict->SetInteger(kSettingPageRangeFrom, settings.ranges()[i].from + 1);
      dict->SetInteger(kSettingPageRangeTo, settings.ranges()[i].to + 1);
      page_range_array->Append(std::move(dict));
    }
  }

  job_settings->SetBoolean(kSettingCollate, settings.collate());
  job_settings->SetInteger(kSettingCopies, 1);
  job_settings->SetInteger(kSettingColor, settings.color());
  job_settings->SetInteger(kSettingDuplexMode, settings.duplex_mode());
  job_settings->SetBoolean(kSettingLandscape, settings.landscape());
  job_settings->SetString(kSettingDeviceName, settings.device_name());
  job_settings->SetInteger(kSettingScaleFactor, 100);
  job_settings->SetBoolean("rasterizePDF", false);

  job_settings->SetInteger("dpi", settings.dpi());
  job_settings->SetInteger("dpiHorizontal", 72);
  job_settings->SetInteger("dpiVertical", 72);

  job_settings->SetBoolean(kSettingPrintToPDF, false);
  job_settings->SetBoolean(kSettingCloudPrintDialog, false);
  job_settings->SetBoolean(kSettingPrintWithPrivet, false);
  job_settings->SetBoolean(kSettingPrintWithExtension, false);

  job_settings->SetBoolean(kSettingShowSystemDialog, false);
  job_settings->SetInteger(kSettingPreviewPageCount, 1);
}


class PrintingContextDelegate : public PrintingContext::Delegate {
 public:
  PrintingContextDelegate(int render_process_id, int render_frame_id);
  ~PrintingContextDelegate() override;

  gfx::NativeView GetParentView() override;
  std::string GetAppLocale() override;

  // Not exposed to PrintingContext::Delegate because of dependency issues.
  content::WebContents* GetWebContents();

 private:
  const int render_process_id_;
  const int render_frame_id_;
};

PrintingContextDelegate::PrintingContextDelegate(int render_process_id,
                                                 int render_frame_id)
    : render_process_id_(render_process_id),
      render_frame_id_(render_frame_id) {}

PrintingContextDelegate::~PrintingContextDelegate() {
}

gfx::NativeView PrintingContextDelegate::GetParentView() {
  content::WebContents* wc = GetWebContents();
  return wc ? wc->GetNativeView() : nullptr;
}

content::WebContents* PrintingContextDelegate::GetWebContents() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* rfh =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
  return rfh ? content::WebContents::FromRenderFrameHost(rfh) : nullptr;
}

std::string PrintingContextDelegate::GetAppLocale() {
  return g_browser_process->GetApplicationLocale();
}

void NotificationCallback(PrintJobWorkerOwner* print_job,
                          JobEventDetails::Type detail_type,
                          PrintedDocument* document,
                          PrintedPage* page) {
  JobEventDetails* details = new JobEventDetails(detail_type, document, page);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PRINT_JOB_EVENT,
      // We know that is is a PrintJob object in this circumstance.
      content::Source<PrintJob>(static_cast<PrintJob*>(print_job)),
      content::Details<JobEventDetails>(details));
}

void PostOnOwnerThread(const scoped_refptr<PrintJobWorkerOwner>& owner,
                       const PrintingContext::PrintSettingsCallback& callback,
                       PrintingContext::Result result) {
  owner->PostTask(FROM_HERE, base::Bind(&HoldRefCallback, owner,
                                        base::Bind(callback, result)));
}

}  // namespace

PrintJobWorker::PrintJobWorker(int render_process_id,
                               int render_frame_id,
                               PrintJobWorkerOwner* owner)
    : owner_(owner), thread_("Printing_Worker"), weak_factory_(this) {
  // The object is created in the IO thread.
  DCHECK(owner_->RunsTasksInCurrentSequence());

  printing_context_delegate_ = base::MakeUnique<PrintingContextDelegate>(
      render_process_id, render_frame_id);
  printing_context_ = PrintingContext::Create(printing_context_delegate_.get());
}

PrintJobWorker::~PrintJobWorker() {
  // The object is normally deleted in the UI thread, but when the user
  // cancels printing or in the case of print preview, the worker is destroyed
  // on the I/O thread.
  DCHECK(owner_->RunsTasksInCurrentSequence());
  Stop();
}

void PrintJobWorker::SetNewOwner(PrintJobWorkerOwner* new_owner) {
  DCHECK(page_number_ == PageNumber::npos());
  owner_ = new_owner;
}

void PrintJobWorker::GetSettings(bool ask_user_for_settings,
                                 int document_page_count,
                                 bool has_selection,
                                 MarginType margin_type,
                                 bool is_scripted,
                                 bool is_modifiable,
                                 const base::string16& device_name) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DCHECK_EQ(page_number_, PageNumber::npos());

  // Recursive task processing is needed for the dialog in case it needs to be
  // destroyed by a task.
  // TODO(thestig): This code is wrong. SetNestableTasksAllowed(true) is needed
  // on the thread where the PrintDlgEx is called, and definitely both calls
  // should happen on the same thread. See http://crbug.com/73466
  // MessageLoop::current()->SetNestableTasksAllowed(true);
  printing_context_->set_margin_type(margin_type);
  printing_context_->set_is_modifiable(is_modifiable);

  // When we delegate to a destination, we don't ask the user for settings.
  // TODO(mad): Ask the destination for settings.
  if (ask_user_for_settings) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&HoldRefCallback, make_scoped_refptr(owner_),
                   base::Bind(&PrintJobWorker::GetSettingsWithUI,
                              base::Unretained(this),
                              document_page_count,
                              has_selection,
                              is_scripted)));
  } else if (!device_name.empty()) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&HoldRefCallback, make_scoped_refptr(owner_),
                   base::Bind(&PrintJobWorker::InitWithDeviceName,
                              base::Unretained(this),
                              device_name)));
  } else {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&HoldRefCallback, make_scoped_refptr(owner_),
                   base::Bind(&PrintJobWorker::UseDefaultSettings,
                              base::Unretained(this))));
  }
}

void PrintJobWorker::SetSettings(
    std::unique_ptr<base::DictionaryValue> new_settings) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(&HoldRefCallback,
                 make_scoped_refptr(owner_),
                 base::Bind(&PrintJobWorker::UpdatePrintSettings,
                            base::Unretained(this),
                            base::Passed(&new_settings))));
}

void PrintJobWorker::UpdatePrintSettings(
    std::unique_ptr<base::DictionaryValue> new_settings) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  PrintingContext::Result result =
      printing_context_->UpdatePrintSettings(*new_settings);
  GetSettingsDone(result);
}

void PrintJobWorker::GetSettingsDone(PrintingContext::Result result) {
  // Most PrintingContext functions may start a message loop and process
  // message recursively, so disable recursive task processing.
  // TODO(thestig): See above comment. SetNestableTasksAllowed(false) needs to
  // be called on the same thread as the previous call.  See
  // http://crbug.com/73466
  // MessageLoop::current()->SetNestableTasksAllowed(false);

  // We can't use OnFailure() here since owner_ may not support notifications.

  // PrintJob will create the new PrintedDocument.
  owner_->PostTask(FROM_HERE,
                   base::Bind(&PrintJobWorkerOwner::GetSettingsDone,
                              make_scoped_refptr(owner_),
                              printing_context_->settings(),
                              result));
}

void PrintJobWorker::GetSettingsWithUI(
    int document_page_count,
    bool has_selection,
    bool is_scripted) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // weak_factory_ creates pointers valid only on owner_ thread.
  printing_context_->AskUserForSettings(
      document_page_count, has_selection, is_scripted,
      base::Bind(&PostOnOwnerThread, make_scoped_refptr(owner_),
                 base::Bind(&PrintJobWorker::GetSettingsDone,
                            weak_factory_.GetWeakPtr())));
}

void PrintJobWorker::UseDefaultSettings() {
  PrintingContext::Result result = printing_context_->UseDefaultSettings();
  GetSettingsDone(result);
}

void PrintJobWorker::InitWithDeviceName(const base::string16& device_name) {
  const auto& settings = printing_context_->settings();
  std::unique_ptr<base::DictionaryValue> dic(new base::DictionaryValue);
  PrintSettingsToJobSettings(settings, dic.get());
  dic->SetString(kSettingDeviceName, device_name);
  UpdatePrintSettings(std::move(dic));
}

void PrintJobWorker::StartPrinting(PrintedDocument* new_document) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DCHECK_EQ(page_number_, PageNumber::npos());
  DCHECK_EQ(document_.get(), new_document);
  DCHECK(document_.get());

  if (!document_.get() || page_number_ != PageNumber::npos() ||
      document_.get() != new_document) {
    return;
  }

  base::string16 document_name =
      printing::SimplifyDocumentTitle(document_->name());
  PrintingContext::Result result =
      printing_context_->NewDocument(document_name);
  if (result != PrintingContext::OK) {
    OnFailure();
    return;
  }

  // Try to print already cached data. It may already have been generated for
  // the print preview.
  OnNewPage();
  // Don't touch this anymore since the instance could be destroyed. It happens
  // if all the pages are printed a one sweep and the client doesn't have a
  // handle to us anymore. There's a timing issue involved between the worker
  // thread and the UI thread. Take no chance.
}

void PrintJobWorker::OnDocumentChanged(PrintedDocument* new_document) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DCHECK_EQ(page_number_, PageNumber::npos());

  if (page_number_ != PageNumber::npos())
    return;

  document_ = new_document;
}

void PrintJobWorker::OnNewPage() {
  if (!document_.get())  // Spurious message.
    return;

  // message_loop() could return NULL when the print job is cancelled.
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  if (page_number_ == PageNumber::npos()) {
    // Find first page to print.
    int page_count = document_->page_count();
    if (!page_count) {
      // We still don't know how many pages the document contains. We can't
      // start to print the document yet since the header/footer may refer to
      // the document's page count.
      return;
    }
    // We have enough information to initialize page_number_.
    page_number_.Init(document_->settings(), page_count);
  }
  DCHECK_NE(page_number_, PageNumber::npos());

  while (true) {
    // Is the page available?
    scoped_refptr<PrintedPage> page = document_->GetPage(page_number_.ToInt());
    if (!page.get()) {
      // We need to wait for the page to be available.
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::Bind(&PrintJobWorker::OnNewPage, weak_factory_.GetWeakPtr()),
          base::TimeDelta::FromMilliseconds(500));
      break;
    }
    // The page is there, print it.
    SpoolPage(page.get());
    ++page_number_;
    if (page_number_ == PageNumber::npos()) {
      OnDocumentDone();
      // Don't touch this anymore since the instance could be destroyed.
      break;
    }
  }
}

void PrintJobWorker::Cancel() {
  // This is the only function that can be called from any thread.
  printing_context_->Cancel();
  // Cannot touch any member variable since we don't know in which thread
  // context we run.
}

bool PrintJobWorker::IsRunning() const {
  return thread_.IsRunning();
}

bool PrintJobWorker::PostTask(const tracked_objects::Location& from_here,
                              const base::Closure& task) {
  if (task_runner_.get())
    return task_runner_->PostTask(from_here, task);
  return false;
}

void PrintJobWorker::StopSoon() {
  thread_.StopSoon();
}

void PrintJobWorker::Stop() {
  thread_.Stop();
}

bool PrintJobWorker::Start() {
  bool result = thread_.Start();
  task_runner_ = thread_.task_runner();
  return result;
}

void PrintJobWorker::OnDocumentDone() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DCHECK_EQ(page_number_, PageNumber::npos());
  DCHECK(document_.get());

  if (printing_context_->DocumentDone() != PrintingContext::OK) {
    OnFailure();
    return;
  }

  owner_->PostTask(FROM_HERE,
                   base::Bind(&NotificationCallback, base::RetainedRef(owner_),
                              JobEventDetails::DOC_DONE,
                              base::RetainedRef(document_), nullptr));

  // Makes sure the variables are reinitialized.
  document_ = NULL;
}

void PrintJobWorker::SpoolPage(PrintedPage* page) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DCHECK_NE(page_number_, PageNumber::npos());

  // Signal everyone that the page is about to be printed.
  owner_->PostTask(
      FROM_HERE,
      base::Bind(&NotificationCallback, base::RetainedRef(owner_),
                 JobEventDetails::NEW_PAGE, base::RetainedRef(document_),
                 base::RetainedRef(page)));

  // Preprocess.
  if (printing_context_->NewPage() != PrintingContext::OK) {
    OnFailure();
    return;
  }

  // Actual printing.
#if defined(OS_WIN) || defined(OS_MACOSX)
  document_->RenderPrintedPage(*page, printing_context_->context());
#elif defined(OS_POSIX)
  document_->RenderPrintedPage(*page, printing_context_.get());
#endif

  // Postprocess.
  if (printing_context_->PageDone() != PrintingContext::OK) {
    OnFailure();
    return;
  }

  // Signal everyone that the page is printed.
  owner_->PostTask(
      FROM_HERE,
      base::Bind(&NotificationCallback, base::RetainedRef(owner_),
                 JobEventDetails::PAGE_DONE, base::RetainedRef(document_),
                 base::RetainedRef(page)));
}

void PrintJobWorker::OnFailure() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  // We may loose our last reference by broadcasting the FAILED event.
  scoped_refptr<PrintJobWorkerOwner> handle(owner_);

  owner_->PostTask(FROM_HERE,
                   base::Bind(&NotificationCallback, base::RetainedRef(owner_),
                              JobEventDetails::FAILED,
                              base::RetainedRef(document_), nullptr));
  Cancel();

  // Makes sure the variables are reinitialized.
  document_ = NULL;
  page_number_ = PageNumber::npos();
}

}  // namespace printing
