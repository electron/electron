// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/printing/printing_message_filter.h"

#include "atom/browser/printing/printing_config_service.h"
#include "base/bind.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "chrome/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

using content::BrowserThread;

namespace atom {

PrintingMessageFilter::PrintingMessageFilter(int render_process_id)
    : BrowserMessageFilter(PrintMsgStart),
      render_process_id_(render_process_id),
      queue_(g_browser_process->print_job_manager()->queue()) {
  DCHECK(queue_);
}

PrintingMessageFilter::~PrintingMessageFilter() {
}

bool PrintingMessageFilter::OnMessageReceived(const IPC::Message& message,
                                              bool* message_was_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(PrintingMessageFilter, message, *message_was_ok)
#if defined(OS_WIN)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DuplicateSection, OnDuplicateSection)
#endif
    IPC_MESSAGE_HANDLER_DELAY_REPLY(PrintHostMsg_GetDefaultPrintSettings,
                                    OnGetDefaultPrintSettings)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(PrintHostMsg_ScriptedPrint, OnScriptedPrint)
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

content::WebContents* PrintingMessageFilter::GetWebContentsForRenderView(
    int render_view_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::RenderViewHost* view = content::RenderViewHost::FromID(
      render_process_id_, render_view_id);
  return view ? content::WebContents::FromRenderViewHost(view) : NULL;
}

void PrintingMessageFilter::GetPrintSettingsForRenderView(
    int render_view_id,
    bool ask_user_for_settings,
    PrintHostMsg_ScriptedPrint_Params params,
    const base::Callback<void(const PrintMsg_PrintPages_Params&)>callback,
    scoped_refptr<printing::PrinterQuery> printer_query) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::WebContents* wc = GetWebContentsForRenderView(render_view_id);
  PrintingConfigService::GetInstance()->GetPrintSettings(
      wc, printer_query, ask_user_for_settings, params, callback);
}

void PrintingMessageFilter::OnGetDefaultPrintSettings(IPC::Message* reply_msg) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  scoped_refptr<printing::PrinterQuery> printer_query;
  if (false) {
    // Reply with NULL query.
    OnGetDefaultPrintSettingsReply(printer_query, reply_msg,
                                   PrintMsg_PrintPages_Params());
    return;
  }
  printer_query = queue_->PopPrinterQuery(0);
  if (!printer_query)
    printer_query = queue_->CreatePrinterQuery();

  // Loads default settings. This is asynchronous, only the IPC message sender
  // will hang until the settings are retrieved.
  PrintHostMsg_ScriptedPrint_Params params;
  params.expected_pages_count = 0;
  params.has_selection = false;
  params.margin_type = printing::DEFAULT_MARGINS;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&PrintingMessageFilter::GetPrintSettingsForRenderView, this,
          reply_msg->routing_id(), false, params,
          base::Bind(&PrintingMessageFilter::OnGetDefaultPrintSettingsReply,
                     this, printer_query, reply_msg),
          printer_query));
}

void PrintingMessageFilter::OnGetDefaultPrintSettingsReply(
    scoped_refptr<printing::PrinterQuery> printer_query,
    IPC::Message* reply_msg,
    const PrintMsg_PrintPages_Params& params) {
  PrintHostMsg_GetDefaultPrintSettings::WriteReplyParams(
      reply_msg, params.params);
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
  scoped_refptr<printing::PrinterQuery> printer_query =
      queue_->PopPrinterQuery(params.cookie);
  if (!printer_query)
    printer_query = queue_->CreatePrinterQuery();
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&PrintingMessageFilter::GetPrintSettingsForRenderView, this,
                 reply_msg->routing_id(), true, params,
                 base::Bind(&PrintingMessageFilter::OnScriptedPrintReply, this,
                            printer_query, reply_msg),
                 printer_query));
}

void PrintingMessageFilter::OnScriptedPrintReply(
    scoped_refptr<printing::PrinterQuery> printer_query,
    IPC::Message* reply_msg,
    const PrintMsg_PrintPages_Params& params) {
  PrintHostMsg_ScriptedPrint::WriteReplyParams(reply_msg, params);
  Send(reply_msg);

  if (params.params.dpi && params.params.document_cookie) {
    queue_->QueuePrinterQuery(printer_query.get());
  } else {
    printer_query->StopWorker();
  }
}

}  // namespace atom
