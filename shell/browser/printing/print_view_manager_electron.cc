// Copyright 2020 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/printing/print_view_manager_electron.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "components/printing/browser/print_composite_client.h"
#include "components/printing/browser/print_manager_utils.h"
#include "components/printing/common/print_params.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "printing/metafile_skia.h"
#include "printing/mojom/print.mojom.h"
#include "printing/print_job_constants.h"
#include "printing/print_settings.h"
#include "printing/printed_document.h"
#include "printing/printing_utils.h"

#if BUILDFLAG(ENABLE_OOP_PRINTING)
#include "chrome/browser/printing/oop_features.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "mojo/public/cpp/bindings/message.h"
#endif

namespace electron {

namespace {

constexpr std::string_view kInvalidSettings = "Invalid printer settings";
constexpr std::string_view kCanceled = "Print job canceled";
constexpr std::string_view kFailed = "Print job failed";

}  // namespace

PrintViewManagerElectron::Job::Job() = default;
PrintViewManagerElectron::Job::Job(Job&&) = default;
PrintViewManagerElectron::Job::~Job() = default;

PrintViewManagerElectron::PrintViewManagerElectron(
    content::WebContents* web_contents)
    : printing::PrintViewManagerBase(web_contents),
      content::WebContentsUserData<PrintViewManagerElectron>(*web_contents) {}

PrintViewManagerElectron::~PrintViewManagerElectron() {
  Finish(false, kFailed);
}

// static
void PrintViewManagerElectron::BindPrintManagerHost(
    mojo::PendingAssociatedReceiver<printing::mojom::PrintManagerHost> receiver,
    content::RenderFrameHost* rfh) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents)
    return;

  auto* print_manager = PrintViewManagerElectron::FromWebContents(web_contents);
  if (!print_manager)
    return;

  print_manager->BindReceiver(std::move(receiver), rfh);
}

void PrintViewManagerElectron::Print(content::RenderFrameHost* rfh,
                                     std::optional<base::DictValue> settings,
                                     bool silent,
                                     PrintCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (job_ || !rfh->IsRenderFrameLive()) {
    std::move(callback).Run(false, std::string(kFailed));
    return;
  }
  job_.emplace();
  job_->id = ++next_job_id_;
  job_->rfh_id = rfh->GetGlobalId();
  job_->callback = std::move(callback);
  job_->silent = silent;
  std::unique_ptr<printing::PrinterQuery> query =
      printing::PrinterQuery::Create(rfh->GetGlobalId());
  printing::PrinterQuery* const query_ptr = query.get();

  // The PDF plugin prints its own document; it must not go to the compositor.
  const bool is_modifiable = !rfh->GetProcess()->IsPdf();
  if (settings) {
    settings->Set(printing::kSettingPreviewModifiable, is_modifiable);
    query_ptr->SetSettings(
        std::move(*settings),
        base::BindOnce(&PrintViewManagerElectron::OnSettingsResolved,
                       weak_factory_.GetWeakPtr(), job_->id, std::move(query)));
    return;
  }
  job_->query = std::move(query);
  if (!RegisterDialogClient(/*assign_to_query=*/true))
    return;
  query = std::move(job_->query);
  query_ptr->GetDefaultSettings(
      base::BindOnce(&PrintViewManagerElectron::OnSettingsResolved,
                     weak_factory_.GetWeakPtr(), job_->id, std::move(query)),
      is_modifiable, /*want_pdf_settings=*/false);
}

bool PrintViewManagerElectron::IsCurrentJob(int id) const {
  return job_ && job_->id == id;
}

bool PrintViewManagerElectron::RegisterDialogClient(bool assign_to_query) {
#if BUILDFLAG(ENABLE_OOP_PRINTING)
  if (printing::ShouldPrintJobOop() && !job_->dialog_client_id) {
    job_->dialog_client_id = printing::PrintBackendServiceManager::GetInstance()
                                 .RegisterQueryWithUiClient();
    if (!job_->dialog_client_id) {
      Finish(false, kFailed);
      return false;
    }
    if (assign_to_query)
      job_->query->SetClientId(*job_->dialog_client_id);
  }
#endif
  return true;
}

void PrintViewManagerElectron::OnSettingsResolved(
    int id,
    std::unique_ptr<printing::PrinterQuery> query) {
  if (!IsCurrentJob(id))
    return;
  job_->query = std::move(query);
  if (job_->query->last_status() != printing::mojom::ResultCode::kSuccess ||
      !job_->query->settings().dpi()) {
    Finish(false, kInvalidSettings);
    return;
  }
  if (job_->silent) {
    RenderDocument();
  } else {
    ShowDialog();
  }
}

void PrintViewManagerElectron::ShowDialog() {
  auto* rfh = content::RenderFrameHost::FromID(job_->rfh_id);
  if (!rfh || !rfh->IsActive()) {
    Finish(false, kFailed);
    return;
  }
  // A query resolved through SetSettings() already holds a print document
  // client; only the service-hosted dialog needs the UI client on it too.
  if (!RegisterDialogClient(
          /*assign_to_query=*/BUILDFLAG(ENABLE_OOP_BASIC_PRINT_DIALOG))) {
    return;
  }
  std::unique_ptr<printing::PrinterQuery> query = std::move(job_->query);
  printing::PrinterQuery* const query_ptr = query.get();
  const printing::PrintSettings& settings = query_ptr->settings();
  query_ptr->GetSettingsFromUser(
      /*expected_page_count=*/0, /*has_selection=*/false,
      settings.margin_type(), /*is_scripted=*/false, settings.is_modifiable(),
      base::BindOnce(&PrintViewManagerElectron::OnDialogDone,
                     weak_factory_.GetWeakPtr(), job_->id, std::move(query)));
}

void PrintViewManagerElectron::OnDialogDone(
    int id,
    std::unique_ptr<printing::PrinterQuery> query) {
  if (!IsCurrentJob(id))
    return;
  job_->query = std::move(query);
  switch (job_->query->last_status()) {
    case printing::mojom::ResultCode::kSuccess:
      RenderDocument();
      return;
    case printing::mojom::ResultCode::kCanceled:
      Finish(false, kCanceled);
      return;
    default:
      Finish(false, kFailed);
      return;
  }
}

void PrintViewManagerElectron::RenderDocument() {
  auto* rfh = content::RenderFrameHost::FromID(job_->rfh_id);
  if (!rfh || !rfh->IsRenderFrameLive()) {
    Finish(false, kFailed);
    return;
  }
  auto params = printing::mojom::PrintPagesParams::New();
  params->params = printing::mojom::PrintParams::New();
  printing::RenderParamsFromPrintSettings(job_->query->settings(),
                                          params->params.get());
  params->params->document_cookie = job_->query->cookie();
  params->pages = job_->query->settings().ranges();
  if (!printing::PrintMsgPrintParamsIsValid(*params->params)) {
    Finish(false, kInvalidSettings);
    return;
  }
  GetPrintRenderFrame(rfh)->PrintWithParams(
      std::move(params),
      base::BindOnce(&PrintViewManagerElectron::OnDocumentRendered,
                     weak_factory_.GetWeakPtr(), job_->id));
}

void PrintViewManagerElectron::OnDocumentRendered(
    int id,
    printing::mojom::PrintRenderFrame::PrintWithParamsResult result) {
  if (!IsCurrentJob(id))
    return;
  auto* rfh = content::RenderFrameHost::FromID(job_->rfh_id);
  if (!result.has_value() || !rfh) {
    Finish(false, kFailed);
    return;
  }
  const uint32_t page_count = (*result)->page_count;
  printing::mojom::DidPrintDocumentParamsPtr params =
      std::move((*result)->params);
  const printing::mojom::DidPrintContentParams& content = *params->content;
  if (!content.metafile_data_region.IsValid() ||
      params->document_cookie != job_->query->cookie()) {
    Finish(false, kFailed);
    return;
  }
  if (printing::LooksLikePdf(content.metafile_data_region.Map()
                                 .GetMemoryAsSpan<const uint8_t>())) {
    base::ReadOnlySharedMemoryRegion pdf =
        std::move(params->content->metafile_data_region);
    SpoolDocument(page_count, std::move(params), std::move(pdf));
    return;
  }
  auto* compositor =
      printing::PrintCompositeClient::FromWebContents(web_contents());
  if (!compositor) {
    Finish(false, kFailed);
    return;
  }
  const int cookie = params->document_cookie;
  auto composited = base::BindOnce(
      &PrintViewManagerElectron::OnDocumentComposited,
      weak_factory_.GetWeakPtr(), id, page_count, std::move(params));
  // `content` stays owned by `params` inside the callback for this call.
  compositor->CompositeDocument(
      cookie, *rfh, content, /*is_pdf=*/false, (*result)->accessibility_tree,
      (*result)->generate_document_outline, std::move(composited));
}

void PrintViewManagerElectron::OnDocumentComposited(
    int id,
    uint32_t page_count,
    printing::mojom::DidPrintDocumentParamsPtr params,
    printing::mojom::PrintCompositor::Status status,
    base::ReadOnlySharedMemoryRegion pdf) {
  if (!IsCurrentJob(id))
    return;
  if (status != printing::mojom::PrintCompositor::Status::kSuccess) {
    Finish(false, kFailed);
    return;
  }
  SpoolDocument(page_count, std::move(params), std::move(pdf));
}

void PrintViewManagerElectron::SpoolDocument(
    uint32_t page_count,
    printing::mojom::DidPrintDocumentParamsPtr params,
    base::ReadOnlySharedMemoryRegion pdf) {
  auto data = base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(pdf);
  auto* print_job_manager = g_browser_process->print_job_manager();
  if (!data || !page_count || !print_job_manager) {
    Finish(false, kFailed);
    return;
  }
  job_->print_job = base::MakeRefCounted<printing::PrintJob>(print_job_manager);
  job_->print_job->Initialize(std::move(job_->query), RenderSourceName(),
                              page_count);
  job_->print_job->AddObserver(job_observer_);
  job_->print_job->StartPrinting();
#if BUILDFLAG(IS_WIN)
  job_->print_job->StartConversionToNativeFormat(
      data, params->page_size, params->content_area, params->physical_offsets,
      web_contents()->GetLastCommittedURL());
#else
  auto metafile = std::make_unique<printing::MetafileSkia>();
  CHECK(metafile->InitFromData(*data));
  job_->print_job->document()->SetDocument(std::move(metafile));
#endif
}

void PrintViewManagerElectron::Finish(bool success, std::string_view reason) {
  if (!job_)
    return;
  Job job = std::move(*job_);
  job_.reset();
  if (job.print_job) {
    job.print_job->RemoveObserver(job_observer_);
    if (!success && job.print_job->is_job_pending())
      job.print_job->Cancel();
  }
#if BUILDFLAG(ENABLE_OOP_PRINTING)
  if (job.dialog_client_id) {
    printing::PrintBackendServiceManager::GetInstance().UnregisterClient(
        *job.dialog_client_id);
  }
#endif
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(job.callback), success,
                                std::string(success ? "" : reason)));
}

void PrintViewManagerElectron::JobObserver::OnJobDone() {
  owner_->Finish(true, "");
}

void PrintViewManagerElectron::JobObserver::OnCanceling() {
  owner_->Finish(false, kCanceled);
}

void PrintViewManagerElectron::JobObserver::OnFailed() {
  owner_->Finish(false, kFailed);
}

void PrintViewManagerElectron::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  PrintViewManagerBase::RenderFrameDeleted(render_frame_host);
  // Once spooling has started the frame is no longer needed.
  if (job_ && !job_->print_job &&
      job_->rfh_id == render_frame_host->GetGlobalId()) {
    Finish(false, kFailed);
  }
}

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
void PrintViewManagerElectron::SetAccessibilityTree(
    int32_t cookie,
    const ui::AXTreeUpdate& accessibility_tree) {
  auto* client =
      printing::PrintCompositeClient::FromWebContents(web_contents());
  if (client) {
    client->SetAccessibilityTree(cookie, accessibility_tree);
  }
}

void PrintViewManagerElectron::GetPrintPreviewParams(
    GetPrintPreviewParamsCallback callback) {
  mojo::ReportBadMessage("Invalid GetPrintPreviewParams Call");
  std::move(callback).Run(nullptr);
}

void PrintViewManagerElectron::SetupScriptedPrintPreview(
    SetupScriptedPrintPreviewCallback callback) {
  mojo::ReportBadMessage("Invalid SetupScriptedPrintPreview Call");
}

void PrintViewManagerElectron::ShowScriptedPrintPreview() {
  mojo::ReportBadMessage("Invalid ShowScriptedPrintPreview Call");
}

void PrintViewManagerElectron::RequestPrintPreview(
    printing::mojom::RequestPrintPreviewParamsPtr params) {
  mojo::ReportBadMessage("Invalid RequestPrintPreview Call");
}

void PrintViewManagerElectron::CheckForCancel(
    const base::UnguessableToken& preview_ui_id,
    int32_t request_id,
    CheckForCancelCallback callback) {
  std::move(callback).Run(false);
}
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

WEB_CONTENTS_USER_DATA_KEY_IMPL(PrintViewManagerElectron);

}  // namespace electron
