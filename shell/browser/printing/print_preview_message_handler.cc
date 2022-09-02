// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/printing/print_preview_message_handler.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/memory/read_only_shared_memory_region.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "components/printing/browser/print_composite_client.h"
#include "components/printing/browser/print_manager_utils.h"
#include "components/services/print_compositor/public/cpp/print_service_mojo_types.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

#include "shell/common/node_includes.h"

using content::BrowserThread;

namespace electron {

namespace {

void StopWorker(int document_cookie) {
  if (document_cookie <= 0)
    return;
  scoped_refptr<printing::PrintQueriesQueue> queue =
      g_browser_process->print_job_manager()->queue();
  std::unique_ptr<printing::PrinterQuery> printer_query =
      queue->PopPrinterQuery(document_cookie);
  if (printer_query.get()) {
    content::GetIOThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(&printing::PrinterQuery::StopWorker,
                                  std::move(printer_query)));
  }
}

}  // namespace

PrintPreviewMessageHandler::PrintPreviewMessageHandler(
    content::WebContents* web_contents)
    : content::WebContentsUserData<PrintPreviewMessageHandler>(*web_contents),
      web_contents_(web_contents) {
  DCHECK(web_contents);
}

PrintPreviewMessageHandler::~PrintPreviewMessageHandler() = default;

void PrintPreviewMessageHandler::MetafileReadyForPrinting(
    printing::mojom::DidPreviewDocumentParamsPtr params,
    int32_t request_id) {
  // Always try to stop the worker.
  StopWorker(params->document_cookie);

  if (params->expected_pages_count == 0) {
    RejectPromise(request_id);
    return;
  }

  const base::ReadOnlySharedMemoryRegion& metafile =
      params->content->metafile_data_region;

  if (printing::IsOopifEnabled()) {
    auto* client =
        printing::PrintCompositeClient::FromWebContents(web_contents_);
    DCHECK(client);

    auto callback = base::BindOnce(
        &PrintPreviewMessageHandler::OnCompositeDocumentToPdfDone,
        weak_ptr_factory_.GetWeakPtr(), request_id);

    client->DoCompleteDocumentToPdf(
        params->document_cookie, params->expected_pages_count,
        mojo::WrapCallbackWithDefaultInvokeIfNotRun(
            std::move(callback),
            printing::mojom::PrintCompositor::Status::kCompositingFailure,
            base::ReadOnlySharedMemoryRegion()));
  } else {
    ResolvePromise(
        request_id,
        base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(metafile));
  }
}

void PrintPreviewMessageHandler::OnPrepareForDocumentToPdfDone(
    int32_t request_id,
    printing::mojom::PrintCompositor::Status status) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (status != printing::mojom::PrintCompositor::Status::kSuccess) {
    LOG(ERROR) << "Preparing document for pdf failed with error " << status;
  }
}

void PrintPreviewMessageHandler::DidPrepareDocumentForPreview(
    int32_t document_cookie,
    int32_t request_id) {
  if (printing::IsOopifEnabled()) {
    auto* client =
        printing::PrintCompositeClient::FromWebContents(web_contents_);
    DCHECK(client);

    if (client->GetIsDocumentConcurrentlyComposited(document_cookie))
      return;

    auto* focused_frame = web_contents_->GetFocusedFrame();
    auto* rfh = focused_frame && focused_frame->HasSelection()
                    ? focused_frame
                    : web_contents_->GetPrimaryMainFrame();

    client->DoPrepareForDocumentToPdf(
        document_cookie, rfh,
        mojo::WrapCallbackWithDefaultInvokeIfNotRun(
            base::BindOnce(
                &PrintPreviewMessageHandler::OnPrepareForDocumentToPdfDone,
                weak_ptr_factory_.GetWeakPtr(), request_id),
            printing::mojom::PrintCompositor::Status::kCompositingFailure));
  }
}

void PrintPreviewMessageHandler::OnCompositeDocumentToPdfDone(
    int32_t request_id,
    printing::mojom::PrintCompositor::Status status,
    base::ReadOnlySharedMemoryRegion region) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (status != printing::mojom::PrintCompositor::Status::kSuccess) {
    LOG(ERROR) << "Compositing pdf failed with error " << status;
    RejectPromise(request_id);
    return;
  }

  ResolvePromise(
      request_id,
      base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(region));
}

void PrintPreviewMessageHandler::OnCompositePdfPageDone(
    int page_number,
    int document_cookie,
    int32_t request_id,
    printing::mojom::PrintCompositor::Status status,
    base::ReadOnlySharedMemoryRegion region) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (status != printing::mojom::PrintCompositor::Status::kSuccess) {
    LOG(ERROR) << "Compositing pdf failed on page: " << page_number
               << " with error: " << status;
  }
}

void PrintPreviewMessageHandler::DidPreviewPage(
    printing::mojom::DidPreviewPageParamsPtr params,
    int32_t request_id) {
  int page_number = params->page_number;
  const printing::mojom::DidPrintContentParams& content = *(params->content);

  if (page_number < printing::FIRST_PAGE_INDEX ||
      !content.metafile_data_region.IsValid()) {
    RejectPromise(request_id);
    return;
  }

  if (printing::IsOopifEnabled()) {
    auto* client =
        printing::PrintCompositeClient::FromWebContents(web_contents_);
    DCHECK(client);

    auto* focused_frame = web_contents_->GetFocusedFrame();
    auto* rfh = focused_frame && focused_frame->HasSelection()
                    ? focused_frame
                    : web_contents_->GetPrimaryMainFrame();

    // Use utility process to convert skia metafile to pdf.
    client->DoCompositePageToPdf(
        params->document_cookie, rfh, content,
        mojo::WrapCallbackWithDefaultInvokeIfNotRun(
            base::BindOnce(&PrintPreviewMessageHandler::OnCompositePdfPageDone,
                           weak_ptr_factory_.GetWeakPtr(), page_number,
                           params->document_cookie, request_id),
            printing::mojom::PrintCompositor::Status::kCompositingFailure,
            base::ReadOnlySharedMemoryRegion()));
  }
}

void PrintPreviewMessageHandler::PrintPreviewFailed(int32_t document_cookie,
                                                    int32_t request_id) {
  StopWorker(document_cookie);

  RejectPromise(request_id);
}

void PrintPreviewMessageHandler::PrintPreviewCancelled(int32_t document_cookie,
                                                       int32_t request_id) {
  StopWorker(document_cookie);

  RejectPromise(request_id);
}

void PrintPreviewMessageHandler::PrintToPDF(
    base::DictionaryValue options,
    gin_helper::Promise<v8::Local<v8::Value>> promise) {
  int request_id;
  options.GetInteger(printing::kPreviewRequestID, &request_id);
  promise_map_.emplace(request_id, std::move(promise));

  auto* focused_frame = web_contents_->GetFocusedFrame();
  auto* rfh = focused_frame && focused_frame->HasSelection()
                  ? focused_frame
                  : web_contents_->GetPrimaryMainFrame();

  if (!print_render_frame_.is_bound()) {
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(&print_render_frame_);
  }
  if (!receiver_.is_bound()) {
    print_render_frame_->SetPrintPreviewUI(
        receiver_.BindNewEndpointAndPassRemote());
  }
  print_render_frame_->PrintPreview(options.GetDict().Clone());
}

gin_helper::Promise<v8::Local<v8::Value>>
PrintPreviewMessageHandler::GetPromise(int request_id) {
  auto it = promise_map_.find(request_id);
  DCHECK(it != promise_map_.end());

  gin_helper::Promise<v8::Local<v8::Value>> promise = std::move(it->second);
  promise_map_.erase(it);

  return promise;
}

void PrintPreviewMessageHandler::ResolvePromise(
    int request_id,
    scoped_refptr<base::RefCountedMemory> data_bytes) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  gin_helper::Promise<v8::Local<v8::Value>> promise = GetPromise(request_id);

  v8::Isolate* isolate = promise.isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(isolate, promise.GetContext()));

  v8::Local<v8::Value> buffer =
      node::Buffer::Copy(isolate,
                         reinterpret_cast<const char*>(data_bytes->front()),
                         data_bytes->size())
          .ToLocalChecked();

  promise.Resolve(buffer);
}

void PrintPreviewMessageHandler::RejectPromise(int request_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  gin_helper::Promise<v8::Local<v8::Value>> promise = GetPromise(request_id);
  promise.RejectWithErrorMessage("Failed to generate PDF");
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PrintPreviewMessageHandler);

}  // namespace electron
