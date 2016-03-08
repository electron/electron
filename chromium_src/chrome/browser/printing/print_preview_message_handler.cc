// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_preview_message_handler.h"

#include <stdint.h>

#include <vector>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/shared_memory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/print_preview_dialog_controller.h"
#include "chrome/browser/printing/print_view_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "printing/page_size_margins.h"
#include "printing/print_job_constants.h"

using content::BrowserThread;
using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintPreviewMessageHandler);

namespace printing {

namespace {

void StopWorker(int document_cookie) {
  if (document_cookie <= 0)
    return;
  scoped_refptr<PrintQueriesQueue> queue =
      g_browser_process->print_job_manager()->queue();
  scoped_refptr<PrinterQuery> printer_query =
      queue->PopPrinterQuery(document_cookie);
  if (printer_query.get()) {
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::Bind(&PrinterQuery::StopWorker,
                                       printer_query));
  }
}

base::RefCountedBytes* GetDataFromHandle(base::SharedMemoryHandle handle,
                                         uint32_t data_size) {
  scoped_ptr<base::SharedMemory> shared_buf(
      new base::SharedMemory(handle, true));
  if (!shared_buf->Map(data_size)) {
    NOTREACHED();
    return nullptr;
  }

  unsigned char* data_begin = static_cast<unsigned char*>(shared_buf->memory());
  std::vector<unsigned char> data(data_begin, data_begin + data_size);
  return base::RefCountedBytes::TakeVector(&data);
}

}  // namespace

PrintPreviewMessageHandler::PrintPreviewMessageHandler(
    WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  DCHECK(web_contents);
}

PrintPreviewMessageHandler::~PrintPreviewMessageHandler() {
}

WebContents* PrintPreviewMessageHandler::GetPrintPreviewDialog() {
  PrintPreviewDialogController* dialog_controller =
      PrintPreviewDialogController::GetInstance();
  if (!dialog_controller)
    return nullptr;
  return dialog_controller->GetPrintPreviewForContents(web_contents());
}

PrintPreviewUI* PrintPreviewMessageHandler::GetPrintPreviewUI() {
  WebContents* dialog = GetPrintPreviewDialog();
  if (!dialog || !dialog->GetWebUI())
    return nullptr;
  return static_cast<PrintPreviewUI*>(dialog->GetWebUI()->GetController());
}

void PrintPreviewMessageHandler::OnRequestPrintPreview(
    const PrintHostMsg_RequestPrintPreview_Params& params) {
  if (params.webnode_only)
    PrintViewManager::FromWebContents(web_contents())->PrintPreviewForWebNode();
  PrintPreviewDialogController::PrintPreview(web_contents());
  PrintPreviewUI::SetInitialParams(GetPrintPreviewDialog(), params);
}

void PrintPreviewMessageHandler::OnDidGetPreviewPageCount(
    const PrintHostMsg_DidGetPreviewPageCount_Params& params) {
  if (params.page_count <= 0) {
    NOTREACHED();
    return;
  }

  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;

  if (!params.is_modifiable || params.clear_preview_data)
    print_preview_ui->ClearAllPreviewData();

  print_preview_ui->OnDidGetPreviewPageCount(params);
}

void PrintPreviewMessageHandler::OnDidPreviewPage(
    const PrintHostMsg_DidPreviewPage_Params& params) {
  int page_number = params.page_number;
  if (page_number < FIRST_PAGE_INDEX || !params.data_size)
    return;

  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;

  base::RefCountedBytes* data_bytes =
      GetDataFromHandle(params.metafile_data_handle, params.data_size);
  DCHECK(data_bytes);

  print_preview_ui->SetPrintPreviewDataForIndex(page_number, data_bytes);
  print_preview_ui->OnDidPreviewPage(page_number, params.preview_request_id);
}

void PrintPreviewMessageHandler::OnMetafileReadyForPrinting(
    const PrintHostMsg_DidPreviewDocument_Params& params) {
  // Always try to stop the worker.
  StopWorker(params.document_cookie);

  if (params.expected_pages_count <= 0) {
    NOTREACHED();
    return;
  }

  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;

  // TODO(joth): This seems like a good match for using RefCountedStaticMemory
  // to avoid the memory copy, but the SetPrintPreviewData call chain below
  // needs updating to accept the RefCountedMemory* base class.
  base::RefCountedBytes* data_bytes =
      GetDataFromHandle(params.metafile_data_handle, params.data_size);
  if (!data_bytes || !data_bytes->size())
    return;

  print_preview_ui->SetPrintPreviewDataForIndex(COMPLETE_PREVIEW_DOCUMENT_INDEX,
                                                data_bytes);
  print_preview_ui->OnPreviewDataIsAvailable(
      params.expected_pages_count, params.preview_request_id);
}

void PrintPreviewMessageHandler::OnPrintPreviewFailed(int document_cookie) {
  StopWorker(document_cookie);

  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;
  print_preview_ui->OnPrintPreviewFailed();
}

void PrintPreviewMessageHandler::OnDidGetDefaultPageLayout(
    const PageSizeMargins& page_layout_in_points,
    const gfx::Rect& printable_area_in_points,
    bool has_custom_page_size_style) {
  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;
  print_preview_ui->OnDidGetDefaultPageLayout(page_layout_in_points,
                                              printable_area_in_points,
                                              has_custom_page_size_style);
}

void PrintPreviewMessageHandler::OnPrintPreviewCancelled(int document_cookie) {
  // Always need to stop the worker.
  StopWorker(document_cookie);
}

void PrintPreviewMessageHandler::OnInvalidPrinterSettings(int document_cookie) {
  StopWorker(document_cookie);
  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;
  print_preview_ui->OnInvalidPrinterSettings();
}

void PrintPreviewMessageHandler::OnSetOptionsFromDocument(
    const PrintHostMsg_SetOptionsFromDocument_Params& params) {
  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;
  print_preview_ui->OnSetOptionsFromDocument(params);
}

bool PrintPreviewMessageHandler::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintPreviewMessageHandler, message)
    IPC_MESSAGE_HANDLER(PrintHostMsg_RequestPrintPreview,
                        OnRequestPrintPreview)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidGetPreviewPageCount,
                        OnDidGetPreviewPageCount)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidPreviewPage,
                        OnDidPreviewPage)
    IPC_MESSAGE_HANDLER(PrintHostMsg_MetafileReadyForPrinting,
                        OnMetafileReadyForPrinting)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewFailed,
                        OnPrintPreviewFailed)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidGetDefaultPageLayout,
                        OnDidGetDefaultPageLayout)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewCancelled,
                        OnPrintPreviewCancelled)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewInvalidPrinterSettings,
                        OnInvalidPrinterSettings)
    IPC_MESSAGE_HANDLER(PrintHostMsg_SetOptionsFromDocument,
                        OnSetOptionsFromDocument)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

}  // namespace printing
