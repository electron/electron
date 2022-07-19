// Copyright 2020 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/printing/print_view_manager_electron.h"

#include <utility>

#include "build/build_config.h"
#include "content/public/browser/web_contents_user_data.h"

namespace electron {

PrintViewManagerElectron::PrintViewManagerElectron(
    content::WebContents* web_contents)
    : PrintViewManagerBase(web_contents),
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
    case PRINT_SUCCESS:
      return std::string();  // no error message
    case PRINTING_FAILED:
      return "Printing failed";
    case INVALID_PRINTER_SETTINGS:
      return "Show invalid printer settings error";
    case INVALID_MEMORY_HANDLE:
      return "Invalid memory handle";
    case METAFILE_MAP_ERROR:
      return "Map to shared memory error";
    case METAFILE_INVALID_HEADER:
      return "Invalid metafile header";
    case METAFILE_GET_DATA_ERROR:
      return "Get data from metafile error";
    case SIMULTANEOUS_PRINT_ACTIVE:
      return "The previous printing job hasn't finished";
    case PAGE_RANGE_SYNTAX_ERROR:
      return "Page range syntax error";
    case PAGE_RANGE_INVALID_RANGE:
      return "Page range is invalid (start > end)";
    case PAGE_COUNT_EXCEEDED:
      return "Page range exceeds page count";
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
    std::move(callback).Run(SIMULTANEOUS_PRINT_ACTIVE,
                            base::MakeRefCounted<base::RefCountedString>());
    return;
  }

  if (!rfh->IsRenderFrameLive()) {
    std::move(callback).Run(PRINTING_FAILED,
                            base::MakeRefCounted<base::RefCountedString>());
    return;
  }

  absl::variant<printing::PageRanges, print_to_pdf::PageRangeError>
      parsed_ranges = print_to_pdf::TextPageRangesToPageRanges(page_ranges);
  if (absl::holds_alternative<print_to_pdf::PageRangeError>(parsed_ranges)) {
    PrintResult print_result;
    switch (absl::get<print_to_pdf::PageRangeError>(parsed_ranges)) {
      case print_to_pdf::PageRangeError::kSyntaxError:
        print_result = PAGE_RANGE_SYNTAX_ERROR;
        break;
      case print_to_pdf::PageRangeError::kInvalidRange:
        print_result = PAGE_RANGE_INVALID_RANGE;
        break;
    }
    std::move(callback).Run(print_result,
                            base::MakeRefCounted<base::RefCountedString>());
    return;
  }

  printing_rfh_ = rfh;
  print_pages_params->pages = absl::get<printing::PageRanges>(parsed_ranges);
  auto cookie = print_pages_params->params->document_cookie;
  set_cookie(cookie);
  headless_jobs_.emplace_back(cookie);
  callback_ = std::move(callback);

  GetPrintRenderFrame(rfh)->PrintWithParams(std::move(print_pages_params));
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

  ReleaseJob(INVALID_PRINTER_SETTINGS);
}

void PrintViewManagerElectron::PrintingFailed(
    int32_t cookie,
    printing::mojom::PrintFailureReason reason) {
  auto entry = std::find(headless_jobs_.begin(), headless_jobs_.end(), cookie);
  if (entry == headless_jobs_.end()) {
    PrintViewManagerBase::PrintingFailed(cookie, reason);
    return;
  }

  ReleaseJob(reason == printing::mojom::PrintFailureReason::kInvalidPageRange
                 ? PAGE_COUNT_EXCEEDED
                 : PRINTING_FAILED);
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
  std::move(callback).Run();
}

void PrintViewManagerElectron::ShowScriptedPrintPreview(
    bool source_is_modifiable) {}

void PrintViewManagerElectron::RequestPrintPreview(
    printing::mojom::RequestPrintPreviewParamsPtr params) {}

void PrintViewManagerElectron::CheckForCancel(int32_t preview_ui_id,
                                              int32_t request_id,
                                              CheckForCancelCallback callback) {
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PrintViewManagerElectron);

}  // namespace electron
