// Copyright 2020 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/printing/print_view_manager_electron.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "components/printing/browser/print_manager_utils.h"
#include "components/printing/browser/print_to_pdf/pdf_print_utils.h"
#include "components/printing/common/print_params.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "printing/mojom/print.mojom.h"
#include "printing/page_range.h"
#include "printing/print_job_constants.h"
#include "printing/print_settings.h"
#include "printing/print_settings_conversion.h"
#include "third_party/abseil-cpp/absl/types/variant.h"

#if BUILDFLAG(ENABLE_OOP_PRINTING)
#include "chrome/browser/printing/oop_features.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "components/printing/browser/print_composite_client.h"
#include "mojo/public/cpp/bindings/message.h"
#endif

namespace electron {

namespace {

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
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

void PrintViewManagerElectron::DidPrintToPdf(
    int cookie,
    PrintToPdfCallback callback,
    print_to_pdf::PdfPrintResult result,
    scoped_refptr<base::RefCountedMemory> memory) {
  std::erase(pdf_jobs_, cookie);
  std::move(callback).Run(result, memory);
}

void PrintViewManagerElectron::PrintToPdf(
    content::RenderFrameHost* rfh,
    const std::string& page_ranges,
    printing::mojom::PrintPagesParamsPtr print_pages_params,
    PrintToPdfCallback callback) {
  // Store cookie in order to track job uniqueness and differentiate
  // between regular and headless print jobs.
  int cookie = print_pages_params->params->document_cookie;
  pdf_jobs_.emplace_back(cookie);

  print_to_pdf::PdfPrintJob::StartJob(
      web_contents(), rfh, GetPrintRenderFrame(rfh), page_ranges,
      std::move(print_pages_params),
      base::BindOnce(&PrintViewManagerElectron::DidPrintToPdf,
                     weak_factory_.GetWeakPtr(), cookie, std::move(callback)));
}

void PrintViewManagerElectron::GetDefaultPrintSettings(
    GetDefaultPrintSettingsCallback callback) {
  // This isn't ideal, but we're not able to access the document cookie here.
  if (pdf_jobs_.size() > 0) {
    LOG(ERROR) << "Scripted print is not supported";
    std::move(callback).Run(printing::mojom::PrintParams::New());
  } else {
    PrintViewManagerBase::GetDefaultPrintSettings(std::move(callback));
  }
}

void PrintViewManagerElectron::ScriptedPrint(
    printing::mojom::ScriptedPrintParamsPtr params,
    ScriptedPrintCallback callback) {
  if (!std::ranges::contains(pdf_jobs_, params->cookie)) {
    PrintViewManagerBase::ScriptedPrint(std::move(params), std::move(callback));
    return;
  }

  auto default_param = printing::mojom::PrintPagesParams::New();
  default_param->params = printing::mojom::PrintParams::New();
  LOG(ERROR) << "Scripted print is not supported";
  std::move(callback).Run(std::move(default_param));
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
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!GetPrintingEnabledBooleanPref()) {
    std::move(callback).Run(nullptr);
    return;
  }

  if (print_preview_settings_.empty()) {
    std::move(callback).Run(nullptr);
    return;
  }

  base::DictValue job_settings = std::move(print_preview_settings_.front());
  print_preview_settings_.pop();
  CHECK(!job_settings.empty());

  std::optional<int> printer_type_value =
      job_settings.FindInt(printing::kSettingPrinterType);
  if (!printer_type_value) {
    std::move(callback).Run(nullptr);
    return;
  }

  auto printer_type =
      static_cast<printing::mojom::PrinterType>(*printer_type_value);
  if (printer_type != printing::mojom::PrinterType::kExtension &&
      printer_type != printing::mojom::PrinterType::kPdf &&
      printer_type != printing::mojom::PrinterType::kLocal) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::unique_ptr<printing::PrintSettings> print_settings =
      printing::PrintSettingsFromJobSettings(job_settings);
  if (!print_settings) {
    std::move(callback).Run(nullptr);
    return;
  }

  bool open_in_external_preview =
      job_settings.contains(printing::kSettingOpenPDFInPreview);
  if (!open_in_external_preview &&
      (printer_type == printing::mojom::PrinterType::kPdf ||
       printer_type == printing::mojom::PrinterType::kExtension)) {
    if (print_settings->page_setup_device_units().printable_area().IsEmpty()) {
      printing::PrinterQuery::
          ApplyDefaultPrintableAreaToVirtualPrinterPrintSettings(
              *print_settings);
    }
  }

#if BUILDFLAG(ENABLE_OOP_PRINTING)
  if (printing::ShouldPrintJobOop() && !query_with_ui_client_id().has_value()) {
    RegisterSystemPrintClient();
  }
#endif

  std::unique_ptr<printing::PrinterQuery> query =
      queue()->CreatePrinterQuery(CurrentTargetFrame().GetGlobalId());
  auto* query_ptr = query.get();
  // We need to clone this before calling SetSettings because some environments
  // evaluate job_settings.Clone() first, and some std::move(job_settings)
  // first; for the former things work correctly but for the latter the cloned
  // value is null.
  auto job_settings_copy = job_settings.Clone();
  query_ptr->SetSettings(
      std::move(job_settings_copy),
      base::BindOnce(&PrintViewManagerElectron::CompleteGetPrintPreviewParams,
                     weak_factory_.GetWeakPtr(), std::move(query),
                     std::move(job_settings), std::move(print_settings),
                     std::move(callback)));
}

void PrintViewManagerElectron::CompleteGetPrintPreviewParams(
    std::unique_ptr<printing::PrinterQuery> printer_query,
    base::DictValue job_settings,
    std::unique_ptr<printing::PrintSettings> print_settings,
    GetPrintPreviewParamsCallback callback) {
  auto settings = printing::mojom::PrintPagesParams::New();
  settings->pages = printing::GetPageRangesFromJobSettings(job_settings);
  settings->params = printing::mojom::PrintParams::New();
  printing::RenderParamsFromPrintSettings(*print_settings,
                                          settings->params.get());
  settings->params->document_cookie =
      printer_query ? printer_query->cookie()
                    : printing::PrintSettings::NewCookie();
  if (!printing::PrintMsgPrintParamsIsValid(*settings->params)) {
    std::move(callback).Run(nullptr);
    return;
  }

  if (printer_query && printer_query->cookie() &&
      printer_query->settings().dpi()) {
    queue()->QueuePrinterQuery(std::move(printer_query));
  }

  set_cookie(settings->params->document_cookie);
  std::move(callback).Run(std::move(settings));
}

void PrintViewManagerElectron::UpdatePrintSettings(
    base::DictValue job_settings,
    UpdatePrintSettingsCallback callback) {
  if (job_settings.empty()) {
    std::move(callback).Run(nullptr);
    return;
  }
  AppendPrintPreviewSettings(std::move(job_settings), /*is_pdf=*/false);
  GetPrintPreviewParams(std::move(callback));
}

void PrintViewManagerElectron::AppendPrintPreviewSettings(
    base::DictValue settings,
    bool is_pdf) {
  CHECK(!settings.empty());
  if (is_pdf) {
    settings.Set(printing::kSettingHeaderFooterEnabled, false);
    settings.Set(printing::kSettingMarginsType,
                 static_cast<int>(printing::mojom::MarginType::kNoMargins));
  }
  print_preview_settings_.push(std::move(settings));
}

void PrintViewManagerElectron::SetupScriptedPrintPreview(
    SetupScriptedPrintPreviewCallback callback) {
  mojo::ReportBadMessage(kInvalidSetupScriptedPrintPreviewCall);
}

void PrintViewManagerElectron::ShowScriptedPrintPreview() {
  mojo::ReportBadMessage(kInvalidShowScriptedPrintPreviewCall);
}

void PrintViewManagerElectron::RequestPrintPreview(
    printing::mojom::RequestPrintPreviewParamsPtr params) {
  mojo::ReportBadMessage(kInvalidRequestPrintPreviewCall);
}

void PrintViewManagerElectron::CheckForCancel(
    const base::UnguessableToken& preview_ui_id,
    int32_t request_id,
    CheckForCancelCallback callback) {
  std::move(callback).Run(false);
}
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

void PrintViewManagerElectron::DidGetPrintedPagesCount(int32_t cookie,
                                                       uint32_t number_pages) {
  if (!std::ranges::contains(pdf_jobs_, cookie)) {
    PrintViewManagerBase::DidGetPrintedPagesCount(cookie, number_pages);
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PrintViewManagerElectron);

}  // namespace electron
