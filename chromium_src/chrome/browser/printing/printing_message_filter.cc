// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/printing_message_filter.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/printing/browser/print_manager_utils.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/child_process_host.h"

#if defined(ENABLE_PRINT_PREVIEW)
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#endif

#if defined(OS_CHROMEOS)
#include <fcntl.h>

#include <map>

#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#endif

#if defined(OS_ANDROID)
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#endif

using content::BrowserThread;

namespace printing {

namespace {

#if defined(OS_ANDROID)
content::WebContents* GetWebContentsForRenderView(int render_process_id,
                                                  int render_view_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::RenderViewHost* view = content::RenderViewHost::FromID(
      render_process_id, render_view_id);
  return view ? content::WebContents::FromRenderViewHost(view) : nullptr;
}

PrintViewManagerBasic* GetPrintManager(int render_process_id,
                                       int render_view_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::WebContents* web_contents =
      GetWebContentsForRenderView(render_process_id, render_view_id);
  return web_contents ? PrintViewManagerBasic::FromWebContents(web_contents)
                      : nullptr;
}
#endif

}  // namespace

PrintingMessageFilter::PrintingMessageFilter(int render_process_id,
                                             Profile* profile)
    : BrowserMessageFilter(PrintMsgStart),
      is_printing_enabled_(new BooleanPrefMember),
      render_process_id_(render_process_id),
      queue_(g_browser_process->print_job_manager()->queue()) {
  DCHECK(queue_.get());
  is_printing_enabled_->Init(prefs::kPrintingEnabled, profile->GetPrefs());
  is_printing_enabled_->MoveToThread(
      BrowserThread::GetMessageLoopProxyForThread(BrowserThread::IO));
}

PrintingMessageFilter::~PrintingMessageFilter() {
}

void PrintingMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message, BrowserThread::ID* thread) {
#if defined(OS_ANDROID)
  if (message.type() == PrintHostMsg_AllocateTempFileForPrinting::ID ||
      message.type() == PrintHostMsg_TempFileForPrintingWritten::ID) {
    *thread = BrowserThread::UI;
  }
#endif
}

bool PrintingMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintingMessageFilter, message)
#if defined(OS_WIN)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DuplicateSection, OnDuplicateSection)
#endif
#if defined(OS_ANDROID)
    IPC_MESSAGE_HANDLER(PrintHostMsg_AllocateTempFileForPrinting,
                        OnAllocateTempFileForPrinting)
    IPC_MESSAGE_HANDLER(PrintHostMsg_TempFileForPrintingWritten,
                        OnTempFileForPrintingWritten)
#endif
    IPC_MESSAGE_HANDLER(PrintHostMsg_IsPrintingEnabled, OnIsPrintingEnabled)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(PrintHostMsg_GetDefaultPrintSettings,
                                    OnGetDefaultPrintSettings)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(PrintHostMsg_ScriptedPrint, OnScriptedPrint)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(PrintHostMsg_UpdatePrintSettings,
                                    OnUpdatePrintSettings)
#if defined(ENABLE_PRINT_PREVIEW)
    IPC_MESSAGE_HANDLER(PrintHostMsg_CheckForCancel, OnCheckForCancel)
#endif
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

#if defined(OS_WIN)
void PrintingMessageFilter::OnDuplicateSection(
    base::SharedMemoryHandle renderer_handle,
    base::SharedMemoryHandle* browser_handle) {
  // Duplicate the handle in this process right now so the memory is kept alive
  // (even if it is not mapped)
  base::SharedMemory shared_buf(renderer_handle, true, PeerHandle());
  shared_buf.GiveToProcess(base::GetCurrentProcessHandle(), browser_handle);
}
#endif

#if defined(OS_ANDROID)
void PrintingMessageFilter::OnAllocateTempFileForPrinting(
    int render_view_id,
    base::FileDescriptor* temp_file_fd,
    int* sequence_number) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  PrintViewManagerBasic* print_view_manager =
      GetPrintManager(render_process_id_, render_view_id);
  if (!print_view_manager)
    return;
  // The file descriptor is originally created in & passed from the Android
  // side, and it will handle the closing.
  temp_file_fd->fd = print_view_manager->file_descriptor().fd;
  temp_file_fd->auto_close = false;
}

void PrintingMessageFilter::OnTempFileForPrintingWritten(int render_view_id,
                                                         int sequence_number) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  PrintViewManagerBasic* print_view_manager =
      GetPrintManager(render_process_id_, render_view_id);
  if (print_view_manager)
    print_view_manager->PdfWritingDone(true);
}
#endif  // defined(OS_ANDROID)

void PrintingMessageFilter::OnIsPrintingEnabled(bool* is_enabled) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  *is_enabled = is_printing_enabled_->GetValue();
}

void PrintingMessageFilter::OnGetDefaultPrintSettings(IPC::Message* reply_msg) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  scoped_refptr<PrinterQuery> printer_query;
  if (!is_printing_enabled_->GetValue()) {
    // Reply with NULL query.
    OnGetDefaultPrintSettingsReply(printer_query, reply_msg);
    return;
  }
  printer_query = queue_->PopPrinterQuery(0);
  if (!printer_query.get()) {
    printer_query =
        queue_->CreatePrinterQuery(render_process_id_, reply_msg->routing_id());
  }

  // Loads default settings. This is asynchronous, only the IPC message sender
  // will hang until the settings are retrieved.
  printer_query->GetSettings(
      PrinterQuery::GetSettingsAskParam::DEFAULTS,
      0,
      false,
      DEFAULT_MARGINS,
      false,
      base::Bind(&PrintingMessageFilter::OnGetDefaultPrintSettingsReply,
                 this,
                 printer_query,
                 reply_msg));
}

void PrintingMessageFilter::OnGetDefaultPrintSettingsReply(
    scoped_refptr<PrinterQuery> printer_query,
    IPC::Message* reply_msg) {
  PrintMsg_Print_Params params;
  if (!printer_query.get() ||
      printer_query->last_status() != PrintingContext::OK) {
    params.Reset();
  } else {
    RenderParamsFromPrintSettings(printer_query->settings(), &params);
    params.document_cookie = printer_query->cookie();
  }
  PrintHostMsg_GetDefaultPrintSettings::WriteReplyParams(reply_msg, params);
  Send(reply_msg);
  // If printing was enabled.
  if (printer_query.get()) {
    // If user hasn't cancelled.
    if (printer_query->cookie() && printer_query->settings().dpi()) {
      queue_->QueuePrinterQuery(printer_query.get());
    } else {
      printer_query->StopWorker();
    }
  }
}

void PrintingMessageFilter::OnScriptedPrint(
    const PrintHostMsg_ScriptedPrint_Params& params,
    IPC::Message* reply_msg) {
  scoped_refptr<PrinterQuery> printer_query =
      queue_->PopPrinterQuery(params.cookie);
  if (!printer_query.get()) {
    printer_query =
        queue_->CreatePrinterQuery(render_process_id_, reply_msg->routing_id());
  }
  printer_query->GetSettings(
      PrinterQuery::GetSettingsAskParam::ASK_USER,
      params.expected_pages_count,
      params.has_selection,
      params.margin_type,
      params.is_scripted,
      base::Bind(&PrintingMessageFilter::OnScriptedPrintReply,
                 this,
                 printer_query,
                 reply_msg));
}

void PrintingMessageFilter::OnScriptedPrintReply(
    scoped_refptr<PrinterQuery> printer_query,
    IPC::Message* reply_msg) {
  PrintMsg_PrintPages_Params params;
#if defined(OS_ANDROID)
  // We need to save the routing ID here because Send method below deletes the
  // |reply_msg| before we can get the routing ID for the Android code.
  int routing_id = reply_msg->routing_id();
#endif
  if (printer_query->last_status() != PrintingContext::OK ||
      !printer_query->settings().dpi()) {
    params.Reset();
  } else {
    RenderParamsFromPrintSettings(printer_query->settings(), &params.params);
    params.params.document_cookie = printer_query->cookie();
    params.pages = PageRange::GetPages(printer_query->settings().ranges());
  }
  PrintHostMsg_ScriptedPrint::WriteReplyParams(reply_msg, params);
  Send(reply_msg);
  if (params.params.dpi && params.params.document_cookie) {
#if defined(OS_ANDROID)
    int file_descriptor;
    const base::string16& device_name = printer_query->settings().device_name();
    if (base::StringToInt(device_name, &file_descriptor)) {
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&PrintingMessageFilter::UpdateFileDescriptor, this,
                     routing_id, file_descriptor));
    }
#endif
    queue_->QueuePrinterQuery(printer_query.get());
  } else {
    printer_query->StopWorker();
  }
}

#if defined(OS_ANDROID)
void PrintingMessageFilter::UpdateFileDescriptor(int render_view_id, int fd) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  PrintViewManagerBasic* print_view_manager =
      GetPrintManager(render_process_id_, render_view_id);
  if (print_view_manager)
    print_view_manager->set_file_descriptor(base::FileDescriptor(fd, false));
}
#endif

void PrintingMessageFilter::OnUpdatePrintSettings(
    int document_cookie, const base::DictionaryValue& job_settings,
    IPC::Message* reply_msg) {
  scoped_ptr<base::DictionaryValue> new_settings(job_settings.DeepCopy());

  scoped_refptr<PrinterQuery> printer_query;
  if (!is_printing_enabled_->GetValue()) {
    // Reply with NULL query.
    OnUpdatePrintSettingsReply(printer_query, reply_msg);
    return;
  }
  printer_query = queue_->PopPrinterQuery(document_cookie);
  if (!printer_query.get()) {
    int host_id = render_process_id_;
    int routing_id = reply_msg->routing_id();
    if (!new_settings->GetInteger(printing::kPreviewInitiatorHostId,
                                  &host_id) ||
        !new_settings->GetInteger(printing::kPreviewInitiatorRoutingId,
                                  &routing_id)) {
      host_id = content::ChildProcessHost::kInvalidUniqueID;
      routing_id = content::ChildProcessHost::kInvalidUniqueID;
    }
    printer_query = queue_->CreatePrinterQuery(host_id, routing_id);
  }
  printer_query->SetSettings(
      std::move(new_settings),
      base::Bind(&PrintingMessageFilter::OnUpdatePrintSettingsReply, this,
                 printer_query, reply_msg));
}

void PrintingMessageFilter::OnUpdatePrintSettingsReply(
    scoped_refptr<PrinterQuery> printer_query,
    IPC::Message* reply_msg) {
  PrintMsg_PrintPages_Params params;
  if (!printer_query.get() ||
      printer_query->last_status() != PrintingContext::OK) {
    params.Reset();
  } else {
    RenderParamsFromPrintSettings(printer_query->settings(), &params.params);
    params.params.document_cookie = printer_query->cookie();
    params.pages = PageRange::GetPages(printer_query->settings().ranges());
  }
  PrintHostMsg_UpdatePrintSettings::WriteReplyParams(
      reply_msg,
      params,
      printer_query.get() &&
          (printer_query->last_status() == printing::PrintingContext::CANCEL));
  Send(reply_msg);
  // If user hasn't cancelled.
  if (printer_query.get()) {
    if (printer_query->cookie() && printer_query->settings().dpi()) {
      queue_->QueuePrinterQuery(printer_query.get());
    } else {
      printer_query->StopWorker();
    }
  }
}

#if defined(ENABLE_PRINT_PREVIEW)
void PrintingMessageFilter::OnCheckForCancel(int32_t preview_ui_id,
                                             int preview_request_id,
                                             bool* cancel) {
  PrintPreviewUI::GetCurrentPrintPreviewStatus(preview_ui_id,
                                               preview_request_id,
                                               cancel);
}
#endif

}  // namespace printing
