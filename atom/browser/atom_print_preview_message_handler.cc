// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/atom_print_preview_message_handler.h"

#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/shared_memory.h"
#include "base/memory/shared_memory_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/print_preview_dialog_controller.h"
#include "chrome/browser/printing/print_view_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "components/printing/browser/print_composite_client.h"
#include "components/printing/browser/print_manager_utils.h"
#include "components/printing/common/print_messages.h"
#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_types.h"
#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "printing/page_size_margins.h"
#include "printing/print_job_constants.h"
#include "printing/print_settings.h"

#include "atom/common/node_includes.h"

using content::BrowserThread;
using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(atom::AtomPrintPreviewMessageHandler);

namespace atom {

namespace {

char* CopyPDFDataOnIOThread(
    const PrintHostMsg_DidPreviewDocument_Params& params) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const PrintHostMsg_DidPrintContent_Params& content = params.content;
  std::unique_ptr<base::SharedMemory> shared_buf(
      new base::SharedMemory(content.metafile_data_handle, true));
  if (!shared_buf->Map(content.data_size))
    return nullptr;
  char* pdf_data = new char[content.data_size];
  memcpy(pdf_data, shared_buf->memory(), content.data_size);
  return pdf_data;
}

void FreeNodeBufferData(char* data, void* hint) {
  delete[] data;
}

}  // namespace

AtomPrintPreviewMessageHandler::AtomPrintPreviewMessageHandler(
    WebContents* web_contents)
    : printing::PrintPreviewMessageHandler(web_contents),
      weak_ptr_factory_(this) {
  DCHECK(web_contents);
}

AtomPrintPreviewMessageHandler::~AtomPrintPreviewMessageHandler() {}

void AtomPrintPreviewMessageHandler::OnMetafileReadyForPrinting(
    content::RenderFrameHost* render_frame_host,
    const PrintHostMsg_DidPreviewDocument_Params& params,
    const PrintHostMsg_PreviewIds& ids) {
  printing::PrintPreviewMessageHandler::OnMetafileReadyForPrinting(
      render_frame_host, params, ids);

  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::IO, FROM_HERE, base::Bind(&CopyPDFDataOnIOThread, params),
      base::Bind(&AtomPrintPreviewMessageHandler::RunPrintToPDFCallback,
                 base::Unretained(this), ids.request_id,
                 params.content.data_size));
}

void AtomPrintPreviewMessageHandler::OnPrintPreviewFailed(
    int document_cookie,
    const PrintHostMsg_PreviewIds& ids) {
  printing::PrintPreviewMessageHandler::OnPrintPreviewFailed(document_cookie,
                                                             ids);

  RunPrintToPDFCallback(ids.request_id, 0, nullptr);
}

void AtomPrintPreviewMessageHandler::PrintToPDF(
    const base::DictionaryValue& options,
    const PrintToPDFCallback& callback) {
  int request_id;
  options.GetInteger(printing::kPreviewRequestID, &request_id);
  print_to_pdf_callback_map_[request_id] = callback;

  content::RenderFrameHost* rfh = web_contents()->GetMainFrame();
  rfh->Send(new PrintMsg_PrintPreview(rfh->GetRoutingID(), options));
}

void AtomPrintPreviewMessageHandler::RunPrintToPDFCallback(int request_id,
                                                           uint32_t data_size,
                                                           char* data) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  if (data) {
    v8::Local<v8::Value> buffer =
        node::Buffer::New(isolate, data, static_cast<size_t>(data_size),
                          &FreeNodeBufferData, nullptr)
            .ToLocalChecked();
    print_to_pdf_callback_map_[request_id].Run(v8::Null(isolate), buffer);
  } else {
    v8::Local<v8::String> error_message =
        v8::String::NewFromUtf8(isolate, "Failed to generate PDF");
    print_to_pdf_callback_map_[request_id].Run(
        v8::Exception::Error(error_message), v8::Null(isolate));
  }
  print_to_pdf_callback_map_.erase(request_id);
}

}  // namespace atom
