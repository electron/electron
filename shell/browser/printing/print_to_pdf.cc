// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/printing/print_to_pdf.h"

#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/values.h"
#include "components/printing/browser/print_to_pdf/pdf_print_result.h"
#include "components/printing/browser/print_to_pdf/pdf_print_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "printing/mojom/print.mojom.h"  // nogncheck
#include "printing/print_job_constants.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/printing/print_view_manager_electron.h"
#include "shell/common/gin_helper/locker.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_util.h"
#include "third_party/abseil-cpp/absl/types/variant.h"

namespace electron {

namespace {

// Constants we use for printToPDF options.
constexpr char kLandscape[] = "landscape";
constexpr char kDisplayHeaderFooter[] = "displayHeaderFooter";
constexpr char kPrintBackground[] = "printBackground";
constexpr char kScale[] = "scale";
constexpr char kPaperWidth[] = "paperWidth";
constexpr char kPaperHeight[] = "paperHeight";
constexpr char kMarginTop[] = "marginTop";
constexpr char kMarginBottom[] = "marginBottom";
constexpr char kMarginLeft[] = "marginLeft";
constexpr char kMarginRight[] = "marginRight";
constexpr char kPageRanges[] = "pageRanges";
constexpr char kHeaderTemplate[] = "headerTemplate";
constexpr char kFooterTemplate[] = "footerTemplate";
constexpr char kPreferCSSPageSize[] = "preferCSSPageSize";
constexpr char kGenerateTaggedPDF[] = "generateTaggedPDF";
constexpr char kGenerateDocumentOutline[] = "generateDocumentOutline";

std::string FindStringOrEmpty(const base::DictValue& dict, const char* key) {
  const std::string* value = dict.FindString(key);
  return value ? *value : std::string();
}

void OnPDFCreated(gin_helper::Promise<v8::Local<v8::Value>> promise,
                  print_to_pdf::PdfPrintResult print_result,
                  scoped_refptr<base::RefCountedMemory> data) {
  if (print_result != print_to_pdf::PdfPrintResult::kPrintSuccess) {
    promise.RejectWithErrorMessage(
        "Failed to generate PDF: " +
        print_to_pdf::PdfPrintResultToString(print_result));
    return;
  }

  v8::Isolate* isolate = promise.isolate();
  gin_helper::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(isolate, promise.GetContext()));

  v8::Local<v8::Value> buffer =
      electron::Buffer::Copy(isolate, *data).ToLocalChecked();

  promise.Resolve(buffer);
}

}  // namespace

// Partially duplicated and modified from
// headless/lib/browser/protocol/page_handler.cc;l=41
v8::Local<v8::Promise> PrintFrameToPDF(content::RenderFrameHost* rfh,
                                       const base::Value& settings) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  const base::DictValue& dict = settings.GetDict();

  // This allows us to track headless printing calls.
  auto unique_id = dict.FindInt(printing::kPreviewRequestID);
  auto landscape = dict.FindBool(kLandscape);
  auto display_header_footer = dict.FindBool(kDisplayHeaderFooter);
  auto print_background = dict.FindBool(kPrintBackground);
  auto scale = dict.FindDouble(kScale);
  auto paper_width = dict.FindDouble(kPaperWidth);
  auto paper_height = dict.FindDouble(kPaperHeight);
  auto margin_top = dict.FindDouble(kMarginTop);
  auto margin_bottom = dict.FindDouble(kMarginBottom);
  auto margin_left = dict.FindDouble(kMarginLeft);
  auto margin_right = dict.FindDouble(kMarginRight);
  auto page_ranges = FindStringOrEmpty(dict, kPageRanges);
  auto header_template = FindStringOrEmpty(dict, kHeaderTemplate);
  auto footer_template = FindStringOrEmpty(dict, kFooterTemplate);
  auto prefer_css_page_size = dict.FindBool(kPreferCSSPageSize);
  auto generate_tagged_pdf = dict.FindBool(kGenerateTaggedPDF);
  auto generate_document_outline = dict.FindBool(kGenerateDocumentOutline);

  absl::variant<printing::mojom::PrintPagesParamsPtr, std::string>
      print_pages_params = print_to_pdf::GetPrintPagesParams(
          rfh->GetLastCommittedURL(), landscape, display_header_footer,
          print_background, scale, paper_width, paper_height, margin_top,
          margin_bottom, margin_left, margin_right,
          std::make_optional(header_template),
          std::make_optional(footer_template), prefer_css_page_size,
          generate_tagged_pdf, generate_document_outline);

  if (absl::holds_alternative<std::string>(print_pages_params)) {
    auto error = absl::get<std::string>(print_pages_params);
    promise.RejectWithErrorMessage("Invalid print parameters: " + error);
    return handle;
  }

  auto* manager = PrintViewManagerElectron::FromWebContents(
      content::WebContents::FromRenderFrameHost(rfh));
  if (!manager) {
    promise.RejectWithErrorMessage("Failed to find print manager");
    return handle;
  }

  auto params = std::move(
      absl::get<printing::mojom::PrintPagesParamsPtr>(print_pages_params));
  params->params->document_cookie = unique_id.value_or(0);

  manager->PrintToPdf(rfh, page_ranges, std::move(params),
                      base::BindOnce(&OnPDFCreated, std::move(promise)));

  return handle;
}

}  // namespace electron
