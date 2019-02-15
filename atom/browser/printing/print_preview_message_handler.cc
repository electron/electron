// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/printing/print_preview_message_handler.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/memory/read_only_shared_memory_region.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "components/printing/browser/print_composite_client.h"
#include "components/printing/browser/print_manager_utils.h"
#include "components/printing/common/print_messages.h"
#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_types.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

#include "atom/common/node_includes.h"

using content::BrowserThread;

namespace atom {

namespace {

void StopWorker(int document_cookie) {
  if (document_cookie <= 0)
    return;
  scoped_refptr<printing::PrintQueriesQueue> queue =
      g_browser_process->print_job_manager()->queue();
  scoped_refptr<printing::PrinterQuery> printer_query =
      queue->PopPrinterQuery(document_cookie);
  if (printer_query.get()) {
    base::PostTaskWithTraits(
        FROM_HERE, {BrowserThread::IO},
        base::BindOnce(&printing::PrinterQuery::StopWorker, printer_query));
  }
}

}  // namespace

PrintPreviewMessageHandler::PrintPreviewMessageHandler(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), weak_ptr_factory_(this) {
  DCHECK(web_contents);
}

PrintPreviewMessageHandler::~PrintPreviewMessageHandler() = default;

bool PrintPreviewMessageHandler::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(PrintPreviewMessageHandler, message,
                                   render_frame_host)
    IPC_MESSAGE_HANDLER(PrintHostMsg_MetafileReadyForPrinting,
                        OnMetafileReadyForPrinting)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  if (handled)
    return true;

  handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintPreviewMessageHandler, message)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewFailed, OnPrintPreviewFailed)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewCancelled,
                        OnPrintPreviewCancelled)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PrintPreviewMessageHandler::OnMetafileReadyForPrinting(
    content::RenderFrameHost* render_frame_host,
    const PrintHostMsg_DidPreviewDocument_Params& params,
    const PrintHostMsg_PreviewIds& ids) {
  // Always try to stop the worker.
  StopWorker(params.document_cookie);

  const PrintHostMsg_DidPrintContent_Params& content = params.content;
  if (!content.metafile_data_region.IsValid() ||
      params.expected_pages_count <= 0) {
    RunPrintToPDFCallback(ids.request_id, nullptr);
    return;
  }

  if (printing::IsOopifEnabled()) {
    auto* client =
        printing::PrintCompositeClient::FromWebContents(web_contents());
    DCHECK(client);
    client->DoCompositeDocumentToPdf(
        params.document_cookie, render_frame_host, content,
        base::BindOnce(&PrintPreviewMessageHandler::OnCompositePdfDocumentDone,
                       weak_ptr_factory_.GetWeakPtr(), ids));
  } else {
    RunPrintToPDFCallback(
        ids.request_id,
        base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(
            content.metafile_data_region));
  }
}

void PrintPreviewMessageHandler::OnCompositePdfDocumentDone(
    const PrintHostMsg_PreviewIds& ids,
    printing::mojom::PdfCompositor::Status status,
    base::ReadOnlySharedMemoryRegion region) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (status != printing::mojom::PdfCompositor::Status::SUCCESS) {
    DLOG(ERROR) << "Compositing pdf failed with error " << status;
    RunPrintToPDFCallback(ids.request_id, nullptr);
    return;
  }

  RunPrintToPDFCallback(
      ids.request_id,
      base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(region));
}

void PrintPreviewMessageHandler::OnPrintPreviewFailed(
    int document_cookie,
    const PrintHostMsg_PreviewIds& ids) {
  StopWorker(document_cookie);

  RunPrintToPDFCallback(ids.request_id, nullptr);
}

void PrintPreviewMessageHandler::OnPrintPreviewCancelled(
    int document_cookie,
    const PrintHostMsg_PreviewIds& ids) {
  StopWorker(document_cookie);

  RunPrintToPDFCallback(ids.request_id, nullptr);
}

void PrintPreviewMessageHandler::PrintToPDF(
    const base::DictionaryValue& options,
    const PrintToPDFCallback& callback) {
  int request_id;
  options.GetInteger(printing::kPreviewRequestID, &request_id);
  print_to_pdf_callback_map_[request_id] = callback;

  auto* focused_frame = web_contents()->GetFocusedFrame();
  auto* rfh = focused_frame && focused_frame->HasSelection()
                  ? focused_frame
                  : web_contents()->GetMainFrame();
  rfh->Send(new PrintMsg_PrintPreview(rfh->GetRoutingID(), options));
}

void PrintPreviewMessageHandler::RunPrintToPDFCallback(
    int request_id,
    scoped_refptr<base::RefCountedMemory> data_bytes) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  if (data_bytes && data_bytes->size()) {
    v8::Local<v8::Value> buffer =
        node::Buffer::Copy(isolate,
                           reinterpret_cast<const char*>(data_bytes->front()),
                           data_bytes->size())
            .ToLocalChecked();
    print_to_pdf_callback_map_[request_id].Run(v8::Null(isolate), buffer);
  } else {
    v8::Local<v8::String> error_message =
        v8::String::NewFromUtf8(isolate, "Failed to generate PDF",
                                v8::NewStringType::kNormal)
            .ToLocalChecked();
    print_to_pdf_callback_map_[request_id].Run(
        v8::Exception::Error(error_message), v8::Null(isolate));
  }
  print_to_pdf_callback_map_.erase(request_id);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PrintPreviewMessageHandler)

}  // namespace atom
