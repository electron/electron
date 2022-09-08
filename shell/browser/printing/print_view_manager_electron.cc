// Copyright 2020 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/printing/print_view_manager_electron.h"

#include <utility>

#include "base/bind.h"
#include "build/build_config.h"
#include "components/printing/browser/print_to_pdf/pdf_print_utils.h"
#include "printing/mojom/print.mojom.h"
#include "printing/page_range.h"
#include "third_party/abseil-cpp/absl/types/variant.h"

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "mojo/public/cpp/bindings/message.h"
#endif

namespace electron {

namespace {

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
constexpr char kInvalidUpdatePrintSettingsCall[] =
    "Invalid UpdatePrintSettings Call";
constexpr char kInvalidSetupScriptedPrintPreviewCall[] =
    "Invalid SetupScriptedPrintPreview Call";
constexpr char kInvalidShowScriptedPrintPreviewCall[] =
    "Invalid ShowScriptedPrintPreview Call";
constexpr char kInvalidRequestPrintPreviewCall[] =
    "Invalid RequestPrintPreview Call";
#endif

}  // namespace

// This file subclasses printing::PrintViewManagerBase
// but the implementations are duplicated from
// components/printing/browser/print_to_pdf/pdf_print_manager.cc.

PrintViewManagerElectron::PrintViewManagerElectron(
    content::WebContents* web_contents)
    : printing::PrintViewManagerBase(web_contents),
      content::WebContentsUserData<PrintViewManagerElectron>(*web_contents) {}

PrintViewManagerElectron::~PrintViewManagerElectron() = default;

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

// static
std::string PrintViewManagerElectron::PrintResultToString(PrintResult result) {
  switch (result) {
    case kPrintSuccess:
      return std::string();  // no error message
    case kPrintFailure:
      return "Printing failed";
    case kInvalidPrinterSettings:
      return "Show invalid printer settings error";
    case kInvalidMemoryHandle:
      return "Invalid memory handle";
    case kMetafileMapError:
      return "Map to shared memory error";
    case kMetafileInvalidHeader:
      return "Invalid metafile header";
    case kMetafileGetDataError:
      return "Get data from metafile error";
    case kSimultaneousPrintActive:
      return "The previous printing job hasn't finished";
    case kPageRangeSyntaxError:
      return "Page range syntax error";
    case kPageRangeInvalidRange:
      return "Page range is invalid (start > end)";
    case kPageCountExceeded:
      return "Page range exceeds page count";
    case kPrintingInProgress:
      return "Page is already being printed";
    default:
      NOTREACHED();
      return "Unknown PrintResult";
  }
}

void PrintViewManagerElectron::PrintToPdf(
    content::RenderFrameHost* rfh,
    const std::string& page_ranges,
    printing::mojom::PrintPagesParamsPtr print_pages_params,
    PrintToPDFCallback callback) {
  DCHECK(callback);

  if (callback_) {
    std::move(callback).Run(kSimultaneousPrintActive,
                            base::MakeRefCounted<base::RefCountedString>());
    return;
  }

  if (!rfh->IsRenderFrameLive()) {
    std::move(callback).Run(kPrintFailure,
                            base::MakeRefCounted<base::RefCountedString>());
    return;
  }

  absl::variant<printing::PageRanges, print_to_pdf::PdfPrintResult>
      parsed_ranges = print_to_pdf::TextPageRangesToPageRanges(page_ranges);
  if (absl::holds_alternative<print_to_pdf::PdfPrintResult>(parsed_ranges)) {
    DCHECK_NE(absl::get<print_to_pdf::PdfPrintResult>(parsed_ranges),
              print_to_pdf::PdfPrintResult::kPrintSuccess);
    std::move(callback).Run(
        static_cast<PrintResult>(
            absl::get<print_to_pdf::PdfPrintResult>(parsed_ranges)),
        base::MakeRefCounted<base::RefCountedString>());
    return;
  }

  printing_rfh_ = rfh;
  print_pages_params->pages = absl::get<printing::PageRanges>(parsed_ranges);
  callback_ = std::move(callback);

  // There is no need for a weak pointer here since the mojo proxy is held
  // in the base class. If we're gone, mojo will discard the callback.
  GetPrintRenderFrame(rfh)->PrintWithParams(
      std::move(print_pages_params),
      base::BindOnce(&PrintViewManagerElectron::OnDidPrintWithParams,
                     base::Unretained(this)));
}

void PrintViewManagerElectron::OnDidPrintWithParams(
    printing::mojom::PrintWithParamsResultPtr result) {
  if (result->is_failure_reason()) {
    switch (result->get_failure_reason()) {
      case printing::mojom::PrintFailureReason::kGeneralFailure:
        FailJob(kPrintFailure);
        return;
      case printing::mojom::PrintFailureReason::kInvalidPageRange:
        FailJob(kPageCountExceeded);
        return;
      case printing::mojom::PrintFailureReason::kPrintingInProgress:
        FailJob(kPrintingInProgress);
        return;
    }
  }

  auto& content = *result->get_params()->content;
  if (!content.metafile_data_region.IsValid()) {
    FailJob(kInvalidMemoryHandle);
    return;
  }

  base::ReadOnlySharedMemoryMapping map = content.metafile_data_region.Map();
  if (!map.IsValid()) {
    FailJob(kMetafileMapError);
    return;
  }

  std::string data =
      std::string(static_cast<const char*>(map.memory()), map.size());
  std::move(callback_).Run(kPrintSuccess,
                           base::RefCountedString::TakeString(&data));

  Reset();
}

void PrintViewManagerElectron::GetDefaultPrintSettings(
    GetDefaultPrintSettingsCallback callback) {
  if (printing_rfh_) {
    LOG(ERROR) << "Scripted print is not supported";
    std::move(callback).Run(printing::mojom::PrintParams::New());
  } else {
    PrintViewManagerBase::GetDefaultPrintSettings(std::move(callback));
  }
}

void PrintViewManagerElectron::ScriptedPrint(
    printing::mojom::ScriptedPrintParamsPtr params,
    ScriptedPrintCallback callback) {
  auto entry =
      std::find(headless_jobs_.begin(), headless_jobs_.end(), params->cookie);
  if (entry == headless_jobs_.end()) {
    PrintViewManagerBase::ScriptedPrint(std::move(params), std::move(callback));
    return;
  }

  auto default_param = printing::mojom::PrintPagesParams::New();
  default_param->params = printing::mojom::PrintParams::New();
  LOG(ERROR) << "Scripted print is not supported";
  std::move(callback).Run(std::move(default_param), /*cancelled*/ false);
}

void PrintViewManagerElectron::ShowInvalidPrinterSettingsError() {
  if (headless_jobs_.size() == 0) {
    PrintViewManagerBase::ShowInvalidPrinterSettingsError();
    return;
  }

  FailJob(kInvalidPrinterSettings);
}

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
void PrintViewManagerElectron::UpdatePrintSettings(
    int32_t cookie,
    base::Value::Dict job_settings,
    UpdatePrintSettingsCallback callback) {
  auto entry = std::find(headless_jobs_.begin(), headless_jobs_.end(), cookie);
  if (entry == headless_jobs_.end()) {
    PrintViewManagerBase::UpdatePrintSettings(cookie, std::move(job_settings),
                                              std::move(callback));
    return;
  }

  mojo::ReportBadMessage(kInvalidUpdatePrintSettingsCall);
}

void PrintViewManagerElectron::SetupScriptedPrintPreview(
    SetupScriptedPrintPreviewCallback callback) {
  mojo::ReportBadMessage(kInvalidSetupScriptedPrintPreviewCall);
}

void PrintViewManagerElectron::ShowScriptedPrintPreview(
    bool source_is_modifiable) {
  mojo::ReportBadMessage(kInvalidShowScriptedPrintPreviewCall);
}

void PrintViewManagerElectron::RequestPrintPreview(
    printing::mojom::RequestPrintPreviewParamsPtr params) {
  mojo::ReportBadMessage(kInvalidRequestPrintPreviewCall);
}

void PrintViewManagerElectron::CheckForCancel(int32_t preview_ui_id,
                                              int32_t request_id,
                                              CheckForCancelCallback callback) {
  std::move(callback).Run(false);
}
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

void PrintViewManagerElectron::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  PrintViewManagerBase::RenderFrameDeleted(render_frame_host);

  if (printing_rfh_ != render_frame_host)
    return;

  FailJob(kPrintFailure);
}

void PrintViewManagerElectron::DidGetPrintedPagesCount(int32_t cookie,
                                                       uint32_t number_pages) {
  auto entry = std::find(headless_jobs_.begin(), headless_jobs_.end(), cookie);
  if (entry == headless_jobs_.end()) {
    PrintViewManagerBase::DidGetPrintedPagesCount(cookie, number_pages);
  }
}

void PrintViewManagerElectron::Reset() {
  printing_rfh_ = nullptr;
  callback_.Reset();
  data_.clear();
}

void PrintViewManagerElectron::FailJob(PrintResult result) {
  DCHECK_NE(result, kPrintSuccess);
  if (callback_) {
    std::move(callback_).Run(result,
                             base::MakeRefCounted<base::RefCountedString>());
  }

  Reset();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PrintViewManagerElectron);

}  // namespace electron
