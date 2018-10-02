// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/printing/print_web_view_helper.h"

#include <algorithm>
#include <string>

#include "base/auto_reset.h"
#include "base/command_line.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram_macros.h"
#include "base/process/process_handle.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/print_messages.h"
#include "content/public/common/web_preferences.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "net/base/escape.h"
#include "printing/pdf_metafile_skia.h"
#include "printing/units.h"
#include "third_party/blink/public/mojom/page/page_visibility_state.mojom.h"
#include "third_party/blink/public/platform/web_double_size.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_local_frame_client.h"
#include "third_party/blink/public/web/web_plugin.h"
#include "third_party/blink/public/web/web_plugin_document.h"
#include "third_party/blink/public/web/web_print_params.h"
#include "third_party/blink/public/web/web_print_scaling_option.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_view.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/base/resource/resource_bundle.h"

using content::WebPreferences;

namespace printing {

namespace {

const double kMinDpi = 1.0;

int GetDPI(const PrintMsg_Print_Params* print_params) {
#if defined(OS_MACOSX)
  // On the Mac, the printable area is in points, don't do any scaling based
  // on dpi.
  return kPointsPerInch;
#else
  return static_cast<int>(print_params->dpi);
#endif  // defined(OS_MACOSX)
}

bool PrintMsg_Print_Params_IsValid(const PrintMsg_Print_Params& params) {
  return !params.content_size.IsEmpty() && !params.page_size.IsEmpty() &&
         !params.printable_area.IsEmpty() && params.document_cookie &&
         params.dpi && params.margin_top >= 0 && params.margin_left >= 0 &&
         params.dpi > kMinDpi && params.document_cookie != 0;
}

PrintMsg_Print_Params GetCssPrintParams(
    blink::WebLocalFrame* frame,
    int page_index,
    const PrintMsg_Print_Params& page_params) {
  PrintMsg_Print_Params page_css_params = page_params;
  int dpi = GetDPI(&page_params);

  blink::WebDoubleSize page_size_in_pixels(
      ConvertUnitDouble(page_params.page_size.width(), dpi, kPixelsPerInch),
      ConvertUnitDouble(page_params.page_size.height(), dpi, kPixelsPerInch));
  int margin_top_in_pixels =
      ConvertUnit(page_params.margin_top, dpi, kPixelsPerInch);
  int margin_right_in_pixels = ConvertUnit(
      page_params.page_size.width() - page_params.content_size.width() -
          page_params.margin_left,
      dpi, kPixelsPerInch);
  int margin_bottom_in_pixels = ConvertUnit(
      page_params.page_size.height() - page_params.content_size.height() -
          page_params.margin_top,
      dpi, kPixelsPerInch);
  int margin_left_in_pixels =
      ConvertUnit(page_params.margin_left, dpi, kPixelsPerInch);

  if (frame) {
    frame->PageSizeAndMarginsInPixels(
        page_index, page_size_in_pixels, margin_top_in_pixels,
        margin_right_in_pixels, margin_bottom_in_pixels, margin_left_in_pixels);
  }

  double new_content_width = page_size_in_pixels.Width() -
                             margin_left_in_pixels - margin_right_in_pixels;
  double new_content_height = page_size_in_pixels.Height() -
                              margin_top_in_pixels - margin_bottom_in_pixels;

  // Invalid page size and/or margins. We just use the default setting.
  if (new_content_width < 1 || new_content_height < 1) {
    CHECK(frame != NULL);
    page_css_params = GetCssPrintParams(NULL, page_index, page_params);
    return page_css_params;
  }

  page_css_params.page_size =
      gfx::Size(ConvertUnit(page_size_in_pixels.Width(), kPixelsPerInch, dpi),
                ConvertUnit(page_size_in_pixels.Height(), kPixelsPerInch, dpi));
  page_css_params.content_size =
      gfx::Size(ConvertUnit(new_content_width, kPixelsPerInch, dpi),
                ConvertUnit(new_content_height, kPixelsPerInch, dpi));

  page_css_params.margin_top =
      ConvertUnit(margin_top_in_pixels, kPixelsPerInch, dpi);
  page_css_params.margin_left =
      ConvertUnit(margin_left_in_pixels, kPixelsPerInch, dpi);
  return page_css_params;
}

double FitPrintParamsToPage(const PrintMsg_Print_Params& page_params,
                            PrintMsg_Print_Params* params_to_fit) {
  double content_width =
      static_cast<double>(params_to_fit->content_size.width());
  double content_height =
      static_cast<double>(params_to_fit->content_size.height());
  int default_page_size_height = page_params.page_size.height();
  int default_page_size_width = page_params.page_size.width();
  int css_page_size_height = params_to_fit->page_size.height();
  int css_page_size_width = params_to_fit->page_size.width();

  double scale_factor = 1.0f;
  if (page_params.page_size == params_to_fit->page_size)
    return scale_factor;

  if (default_page_size_width < css_page_size_width ||
      default_page_size_height < css_page_size_height) {
    double ratio_width =
        static_cast<double>(default_page_size_width) / css_page_size_width;
    double ratio_height =
        static_cast<double>(default_page_size_height) / css_page_size_height;
    scale_factor = ratio_width < ratio_height ? ratio_width : ratio_height;
    content_width *= scale_factor;
    content_height *= scale_factor;
  }
  params_to_fit->margin_top = static_cast<int>(
      (default_page_size_height - css_page_size_height * scale_factor) / 2 +
      (params_to_fit->margin_top * scale_factor));
  params_to_fit->margin_left = static_cast<int>(
      (default_page_size_width - css_page_size_width * scale_factor) / 2 +
      (params_to_fit->margin_left * scale_factor));
  params_to_fit->content_size = gfx::Size(static_cast<int>(content_width),
                                          static_cast<int>(content_height));
  params_to_fit->page_size = page_params.page_size;
  return scale_factor;
}

void CalculatePageLayoutFromPrintParams(
    const PrintMsg_Print_Params& params,
    PageSizeMargins* page_layout_in_points) {
  int dpi = GetDPI(&params);
  int content_width = params.content_size.width();
  int content_height = params.content_size.height();

  int margin_bottom =
      params.page_size.height() - content_height - params.margin_top;
  int margin_right =
      params.page_size.width() - content_width - params.margin_left;

  page_layout_in_points->content_width =
      ConvertUnit(content_width, dpi, kPointsPerInch);
  page_layout_in_points->content_height =
      ConvertUnit(content_height, dpi, kPointsPerInch);
  page_layout_in_points->margin_top =
      ConvertUnit(params.margin_top, dpi, kPointsPerInch);
  page_layout_in_points->margin_right =
      ConvertUnit(margin_right, dpi, kPointsPerInch);
  page_layout_in_points->margin_bottom =
      ConvertUnit(margin_bottom, dpi, kPointsPerInch);
  page_layout_in_points->margin_left =
      ConvertUnit(params.margin_left, dpi, kPointsPerInch);
}

void EnsureOrientationMatches(const PrintMsg_Print_Params& css_params,
                              PrintMsg_Print_Params* page_params) {
  if ((page_params->page_size.width() > page_params->page_size.height()) ==
      (css_params.page_size.width() > css_params.page_size.height())) {
    return;
  }

  // Swap the |width| and |height| values.
  page_params->page_size.SetSize(page_params->page_size.height(),
                                 page_params->page_size.width());
  page_params->content_size.SetSize(page_params->content_size.height(),
                                    page_params->content_size.width());
  page_params->printable_area.set_size(
      gfx::Size(page_params->printable_area.height(),
                page_params->printable_area.width()));
}

void ComputeWebKitPrintParamsInDesiredDpi(
    const PrintMsg_Print_Params& print_params,
    blink::WebPrintParams* webkit_print_params) {
  int dpi = GetDPI(&print_params);
  webkit_print_params->printer_dpi = dpi;
  webkit_print_params->print_scaling_option = print_params.print_scaling_option;

  webkit_print_params->print_content_area.width =
      ConvertUnit(print_params.content_size.width(), dpi, kPointsPerInch);
  webkit_print_params->print_content_area.height =
      ConvertUnit(print_params.content_size.height(), dpi, kPointsPerInch);

  webkit_print_params->printable_area.x =
      ConvertUnit(print_params.printable_area.x(), dpi, kPointsPerInch);
  webkit_print_params->printable_area.y =
      ConvertUnit(print_params.printable_area.y(), dpi, kPointsPerInch);
  webkit_print_params->printable_area.width =
      ConvertUnit(print_params.printable_area.width(), dpi, kPointsPerInch);
  webkit_print_params->printable_area.height =
      ConvertUnit(print_params.printable_area.height(), dpi, kPointsPerInch);

  webkit_print_params->paper_size.width =
      ConvertUnit(print_params.page_size.width(), dpi, kPointsPerInch);
  webkit_print_params->paper_size.height =
      ConvertUnit(print_params.page_size.height(), dpi, kPointsPerInch);
}

blink::WebPlugin* GetPlugin(const blink::WebLocalFrame* frame) {
  return frame->GetDocument().IsPluginDocument()
             ? frame->GetDocument().To<blink::WebPluginDocument>().Plugin()
             : nullptr;
}

bool PrintingNodeOrPdfFrame(const blink::WebLocalFrame* frame,
                            const blink::WebNode& node) {
  if (!node.IsNull())
    return true;
  blink::WebPlugin* plugin = GetPlugin(frame);
  return plugin && plugin->SupportsPaginatedPrint();
}

MarginType GetMarginsForPdf(blink::WebLocalFrame* frame,
                            const blink::WebNode& node) {
  if (frame->IsPrintScalingDisabledForPlugin(node))
    return NO_MARGINS;
  else
    return PRINTABLE_AREA_MARGINS;
}

PrintMsg_Print_Params CalculatePrintParamsForCss(
    blink::WebLocalFrame* frame,
    int page_index,
    const PrintMsg_Print_Params& page_params,
    bool ignore_css_margins,
    bool fit_to_page,
    double* scale_factor) {
  PrintMsg_Print_Params css_params =
      GetCssPrintParams(frame, page_index, page_params);

  PrintMsg_Print_Params params = page_params;
  EnsureOrientationMatches(css_params, &params);

  if (ignore_css_margins && fit_to_page)
    return params;

  PrintMsg_Print_Params result_params = css_params;
  if (ignore_css_margins) {
    result_params.margin_top = params.margin_top;
    result_params.margin_left = params.margin_left;

    DCHECK(!fit_to_page);
    // Since we are ignoring the margins, the css page size is no longer
    // valid.
    int default_margin_right = params.page_size.width() -
                               params.content_size.width() - params.margin_left;
    int default_margin_bottom = params.page_size.height() -
                                params.content_size.height() -
                                params.margin_top;
    result_params.content_size =
        gfx::Size(result_params.page_size.width() - result_params.margin_left -
                      default_margin_right,
                  result_params.page_size.height() - result_params.margin_top -
                      default_margin_bottom);
  }

  if (fit_to_page) {
    double factor = FitPrintParamsToPage(params, &result_params);
    if (scale_factor)
      *scale_factor = factor;
  }
  return result_params;
}

}  // namespace

FrameReference::FrameReference(blink::WebLocalFrame* frame) {
  Reset(frame);
}

FrameReference::FrameReference() {
  Reset(NULL);
}

FrameReference::~FrameReference() {}

void FrameReference::Reset(blink::WebLocalFrame* frame) {
  if (frame) {
    view_ = frame->View();
    frame_ = frame;
  } else {
    view_ = NULL;
    frame_ = NULL;
  }
}

blink::WebLocalFrame* FrameReference::GetFrame() {
  if (view_ == NULL || frame_ == NULL)
    return NULL;
  for (blink::WebFrame* frame = view_->MainFrame(); frame != NULL;
       frame = frame->TraverseNext()) {
    if (frame == frame_)
      return frame_;
  }
  return NULL;
}

blink::WebView* FrameReference::view() {
  return view_;
}

// static - Not anonymous so that platform implementations can use it.
float PrintWebViewHelper::RenderPageContent(blink::WebLocalFrame* frame,
                                            int page_number,
                                            const gfx::Rect& canvas_area,
                                            const gfx::Rect& content_area,
                                            double scale_factor,
                                            cc::PaintCanvas* canvas) {
  cc::PaintCanvasAutoRestore auto_restore(canvas, true);
  canvas->translate((content_area.x() - canvas_area.x()) / scale_factor,
                    (content_area.y() - canvas_area.y()) / scale_factor);
  return frame->PrintPage(page_number, canvas);
}

// Class that calls the Begin and End print functions on the frame and changes
// the size of the view temporarily to support full page printing..
class PrepareFrameAndViewForPrint : public blink::WebViewClient,
                                    public blink::WebLocalFrameClient,
                                    public blink::WebWidgetClient {
 public:
  PrepareFrameAndViewForPrint(const PrintMsg_Print_Params& params,
                              blink::WebLocalFrame* frame,
                              const blink::WebNode& node,
                              bool ignore_css_margins);
  ~PrepareFrameAndViewForPrint() override;

  // Optional. Replaces |frame_| with selection if needed. Will call |on_ready|
  // when completed.
  void CopySelectionIfNeeded(const WebPreferences& preferences,
                             const base::Closure& on_ready);

  // Prepares frame for printing.
  void StartPrinting();

  blink::WebWidgetClient* WidgetClient() override { return this; }

  blink::WebLocalFrame* frame() { return frame_.GetFrame(); }

  const blink::WebNode& node() const { return node_to_print_; }

  int GetExpectedPageCount() const { return expected_pages_count_; }

  void FinishPrinting();

  bool IsLoadingSelection() {
    // It's not selection if not |owns_web_view_|.
    return owns_web_view_ && frame() && frame()->IsLoading();
  }

 protected:
  // blink::WebViewClient override:
  void DidStopLoading() override;
  bool AllowsBrokenNullLayerTreeView() const override;

  // blink::WebLocalFrameClient:
  blink::WebLocalFrame* CreateChildFrame(
      blink::WebLocalFrame* parent,
      blink::WebTreeScopeType scope,
      const blink::WebString& name,
      const blink::WebString& unique_name,
      blink::WebSandboxFlags sandbox_flags,
      const blink::ParsedFeaturePolicy& container_policy,
      const blink::WebFrameOwnerProperties& frame_owner_properties) override;

 private:
  void CallOnReady();
  void ResizeForPrinting();
  void RestoreSize();
  void CopySelection(const WebPreferences& preferences);

  FrameReference frame_;
  blink::WebNode node_to_print_;
  bool owns_web_view_;
  blink::WebPrintParams web_print_params_;
  gfx::Size prev_view_size_;
  gfx::Size prev_scroll_offset_;
  int expected_pages_count_;
  base::Closure on_ready_;
  bool should_print_backgrounds_;
  bool should_print_selection_only_;
  bool is_printing_started_;

  base::WeakPtrFactory<PrepareFrameAndViewForPrint> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrepareFrameAndViewForPrint);
};

PrepareFrameAndViewForPrint::PrepareFrameAndViewForPrint(
    const PrintMsg_Print_Params& params,
    blink::WebLocalFrame* frame,
    const blink::WebNode& node,
    bool ignore_css_margins)
    : frame_(frame),
      node_to_print_(node),
      owns_web_view_(false),
      expected_pages_count_(0),
      should_print_backgrounds_(params.should_print_backgrounds),
      should_print_selection_only_(params.selection_only),
      is_printing_started_(false),
      weak_ptr_factory_(this) {
  PrintMsg_Print_Params print_params = params;
  if (!should_print_selection_only_ ||
      !PrintingNodeOrPdfFrame(frame, node_to_print_)) {
    bool fit_to_page = ignore_css_margins &&
                       print_params.print_scaling_option ==
                           blink::kWebPrintScalingOptionFitToPrintableArea;
    ComputeWebKitPrintParamsInDesiredDpi(params, &web_print_params_);
    frame->PrintBegin(web_print_params_, node_to_print_);
    print_params = CalculatePrintParamsForCss(
        frame, 0, print_params, ignore_css_margins, fit_to_page, NULL);
    frame->PrintEnd();
  }
  ComputeWebKitPrintParamsInDesiredDpi(print_params, &web_print_params_);
}

PrepareFrameAndViewForPrint::~PrepareFrameAndViewForPrint() {
  FinishPrinting();
}

void PrepareFrameAndViewForPrint::ResizeForPrinting() {
  // Layout page according to printer page size. Since WebKit shrinks the
  // size of the page automatically (from 125% to 200%) we trick it to
  // think the page is 125% larger so the size of the page is correct for
  // minimum (default) scaling.
  // This is important for sites that try to fill the page.
  // The 1.25 value is |printingMinimumShrinkFactor|.
  gfx::Size print_layout_size(web_print_params_.print_content_area.width,
                              web_print_params_.print_content_area.height);
  print_layout_size.set_height(
      static_cast<int>(static_cast<double>(print_layout_size.height()) * 1.25));

  if (!frame())
    return;

  // Backup size and offset if it's a local frame.
  blink::WebView* web_view = frame_.view();
  if (blink::WebFrame* web_frame = web_view->MainFrame()) {
    if (web_frame->IsWebLocalFrame())
      prev_scroll_offset_ = web_frame->ToWebLocalFrame()->GetScrollOffset();
  }
  prev_view_size_ = web_view->Size();

  web_view->Resize(print_layout_size);
}

void PrepareFrameAndViewForPrint::StartPrinting() {
  ResizeForPrinting();
  blink::WebView* web_view = frame_.view();
  web_view->GetSettings()->SetShouldPrintBackgrounds(should_print_backgrounds_);
  expected_pages_count_ =
      frame()->PrintBegin(web_print_params_, node_to_print_);
  is_printing_started_ = true;
}

void PrepareFrameAndViewForPrint::CopySelectionIfNeeded(
    const WebPreferences& preferences,
    const base::Closure& on_ready) {
  on_ready_ = on_ready;
  if (should_print_selection_only_) {
    CopySelection(preferences);
  } else {
    // Call immediately, async call crashes scripting printing.
    CallOnReady();
  }
}

void PrepareFrameAndViewForPrint::CopySelection(
    const WebPreferences& preferences) {
  ResizeForPrinting();
  std::string url_str = "data:text/html;charset=utf-8,";
  url_str.append(
      net::EscapeQueryParamValue(frame()->SelectionAsMarkup().Utf8(), false));
  RestoreSize();
  // Create a new WebView with the same settings as the current display one.
  // Except that we disable javascript (don't want any active content running
  // on the page).
  WebPreferences prefs = preferences;
  prefs.javascript_enabled = false;

  blink::WebView* web_view = blink::WebView::Create(
      this, this, blink::mojom::PageVisibilityState::kVisible, nullptr);
  owns_web_view_ = true;
  content::RenderView::ApplyWebPreferences(prefs, web_view);
  blink::WebLocalFrame* main_frame =
      blink::WebLocalFrame::CreateMainFrame(web_view, this, nullptr, nullptr);
  blink::WebFrameWidget::Create(this, main_frame);
  frame_.Reset(web_view->MainFrame()->ToWebLocalFrame());
  node_to_print_.Reset();

  // When loading is done this will call DidStopLoading() and that will do the
  // actual printing.
  frame()->StartNavigation(blink::WebURLRequest(GURL(url_str)));
}

bool PrepareFrameAndViewForPrint::AllowsBrokenNullLayerTreeView() const {
  return true;
}

void PrepareFrameAndViewForPrint::DidStopLoading() {
  DCHECK(!on_ready_.is_null());
  // Don't call callback here, because it can delete |this| and WebView that is
  // called DidStopLoading.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&PrepareFrameAndViewForPrint::CallOnReady,
                            weak_ptr_factory_.GetWeakPtr()));
}

blink::WebLocalFrame* PrepareFrameAndViewForPrint::CreateChildFrame(
    blink::WebLocalFrame* parent,
    blink::WebTreeScopeType scope,
    const blink::WebString& name,
    const blink::WebString& unique_name,
    blink::WebSandboxFlags sandbox_flags,
    const blink::ParsedFeaturePolicy& container_policy,
    const blink::WebFrameOwnerProperties& frame_owner_properties) {
  blink::WebLocalFrame* frame = parent->CreateLocalChild(scope, this, nullptr);
  return frame;
}

void PrepareFrameAndViewForPrint::CallOnReady() {
  return on_ready_.Run();  // Can delete |this|.
}

void PrepareFrameAndViewForPrint::RestoreSize() {
  if (!frame())
    return;

  blink::WebView* web_view = frame_.GetFrame()->View();
  web_view->Resize(prev_view_size_);
  if (blink::WebFrame* web_frame = web_view->MainFrame()) {
    if (web_frame->IsWebLocalFrame())
      web_frame->ToWebLocalFrame()->SetScrollOffset(prev_scroll_offset_);
  }
}

void PrepareFrameAndViewForPrint::FinishPrinting() {
  blink::WebLocalFrame* frame = frame_.GetFrame();
  if (frame) {
    blink::WebView* web_view = frame->View();
    if (is_printing_started_) {
      is_printing_started_ = false;
      frame->PrintEnd();
      if (!owns_web_view_) {
        web_view->GetSettings()->SetShouldPrintBackgrounds(false);
        RestoreSize();
      }
    }
    if (owns_web_view_) {
      DCHECK(!frame->IsLoading());
      owns_web_view_ = false;
      web_view->Close();
    }
  }
  frame_.Reset(NULL);
  on_ready_.Reset();
}

PrintWebViewHelper::PrintWebViewHelper(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      content::RenderFrameObserverTracker<PrintWebViewHelper>(render_frame),
      reset_prep_frame_view_(false),
      is_print_ready_metafile_sent_(false),
      ignore_css_margins_(false),
      is_scripted_printing_blocked_(false),
      notify_browser_of_print_failure_(true),
      print_for_preview_(false),
      print_node_in_progress_(false),
      is_loading_(false),
      is_scripted_preview_delayed_(false),
      ipc_nesting_level_(0),
      weak_ptr_factory_(this) {}

PrintWebViewHelper::~PrintWebViewHelper() {}

// Prints |frame| which called window.print().
void PrintWebViewHelper::ScriptedPrint(bool user_initiated) {
  Print(render_frame()->GetWebFrame(), blink::WebNode());
}

bool PrintWebViewHelper::OnMessageReceived(const IPC::Message& message) {
  // The class is not designed to handle recursive messages. This is not
  // expected during regular flow. However, during rendering of content for
  // printing, lower level code may run nested message loop. E.g. PDF may has
  // script to show message box http://crbug.com/502562. In that moment browser
  // may receive updated printer capabilities and decide to restart print
  // preview generation. When this happened message handling function may
  // choose to ignore message or safely crash process.
  ++ipc_nesting_level_;

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintWebViewHelper, message)
    IPC_MESSAGE_HANDLER(PrintMsg_PrintPages, OnPrintPages)
    IPC_MESSAGE_HANDLER(PrintMsg_PrintingDone, OnPrintingDone)
    IPC_MESSAGE_HANDLER(PrintMsg_PrintPreview, OnPrintPreview)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  --ipc_nesting_level_;
  return handled;
}

void PrintWebViewHelper::OnDestruct() {
  delete this;
}

#if !defined(DISABLE_BASIC_PRINTING)
void PrintWebViewHelper::OnPrintPages(bool silent,
                                      bool print_background,
                                      const base::string16& device_name) {
  if (ipc_nesting_level_ > 1)
    return;

  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  Print(frame, blink::WebNode(), silent, print_background, device_name);
}
#endif  // !DISABLE_BASIC_PRINTING

void PrintWebViewHelper::GetPageSizeAndContentAreaFromPageLayout(
    const PageSizeMargins& page_layout_in_points,
    gfx::Size* page_size,
    gfx::Rect* content_area) {
  *page_size = gfx::Size(
      page_layout_in_points.content_width + page_layout_in_points.margin_right +
          page_layout_in_points.margin_left,
      page_layout_in_points.content_height + page_layout_in_points.margin_top +
          page_layout_in_points.margin_bottom);
  *content_area = gfx::Rect(page_layout_in_points.margin_left,
                            page_layout_in_points.margin_top,
                            page_layout_in_points.content_width,
                            page_layout_in_points.content_height);
}

void PrintWebViewHelper::UpdateFrameMarginsCssInfo(
    const base::DictionaryValue& settings) {
  int margins_type = 0;
  if (!settings.GetInteger(kSettingMarginsType, &margins_type))
    margins_type = DEFAULT_MARGINS;
  ignore_css_margins_ = (margins_type != DEFAULT_MARGINS);
}

void PrintWebViewHelper::OnPrintingDone(bool success) {
  notify_browser_of_print_failure_ = false;
  if (!success)
    LOG(ERROR) << "Failure in OnPrintingDone";
  DidFinishPrinting(success ? OK : FAIL_PRINT);
}

void PrintWebViewHelper::OnPrintPreview(const base::DictionaryValue& settings) {
  if (ipc_nesting_level_ > 1)
    return;

  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();

  print_preview_context_.InitWithFrame(frame);
  if (!print_preview_context_.source_frame()) {
    DidFinishPrinting(FAIL_PREVIEW);
    return;
  }

  if (!UpdatePrintSettings(print_preview_context_.source_frame(),
                           print_preview_context_.source_node(), settings)) {
    if (print_preview_context_.last_error() != PREVIEW_ERROR_BAD_SETTING) {
      DidFinishPrinting(INVALID_SETTINGS);
    } else {
      DidFinishPrinting(FAIL_PREVIEW);
    }
    return;
  }
  is_print_ready_metafile_sent_ = false;
  PrepareFrameForPreviewDocument();
}

void PrintWebViewHelper::PrepareFrameForPreviewDocument() {
  reset_prep_frame_view_ = false;

  if (!print_pages_params_) {
    DidFinishPrinting(FAIL_PREVIEW);
    return;
  }

  // Don't reset loading frame or WebKit will fail assert. Just retry when
  // current selection is loaded.
  if (prep_frame_view_ && prep_frame_view_->IsLoadingSelection()) {
    reset_prep_frame_view_ = true;
    return;
  }

  const PrintMsg_Print_Params& print_params = print_pages_params_->params;
  prep_frame_view_.reset(new PrepareFrameAndViewForPrint(
      print_params, print_preview_context_.source_frame(),
      print_preview_context_.source_node(), ignore_css_margins_));
  prep_frame_view_->CopySelectionIfNeeded(
      render_frame()->GetWebkitPreferences(),
      base::Bind(&PrintWebViewHelper::OnFramePreparedForPreviewDocument,
                 base::Unretained(this)));
}

void PrintWebViewHelper::OnFramePreparedForPreviewDocument() {
  if (reset_prep_frame_view_) {
    PrepareFrameForPreviewDocument();
    return;
  }
  DidFinishPrinting(CreatePreviewDocument() ? OK : FAIL_PREVIEW);
}

bool PrintWebViewHelper::CreatePreviewDocument() {
  if (!print_pages_params_)
    return false;

  const PrintMsg_Print_Params& print_params = print_pages_params_->params;
  const std::vector<int>& pages = print_pages_params_->pages;

  if (!print_preview_context_.CreatePreviewDocument(prep_frame_view_.release(),
                                                    pages)) {
    return false;
  }

  while (!print_preview_context_.IsFinalPageRendered()) {
    int page_number = print_preview_context_.GetNextPageNumber();
    DCHECK_GE(page_number, 0);
    if (!RenderPreviewPage(page_number, print_params))
      return false;

    // We must call PrepareFrameAndViewForPrint::FinishPrinting() (by way of
    // print_preview_context_.AllPagesRendered()) before calling
    // FinalizePrintReadyDocument() when printing a PDF because the plugin
    // code does not generate output until we call FinishPrinting().  We do not
    // generate draft pages for PDFs, so IsFinalPageRendered() and
    // IsLastPageOfPrintReadyMetafile() will be true in the same iteration of
    // the loop.
    if (print_preview_context_.IsFinalPageRendered())
      print_preview_context_.AllPagesRendered();

    if (print_preview_context_.IsLastPageOfPrintReadyMetafile()) {
      DCHECK(print_preview_context_.IsModifiable() ||
             print_preview_context_.IsFinalPageRendered());
      if (!FinalizePrintReadyDocument())
        return false;
    }
  }
  print_preview_context_.Finished();
  return true;
}

bool PrintWebViewHelper::FinalizePrintReadyDocument() {
  DCHECK(!is_print_ready_metafile_sent_);
  print_preview_context_.FinalizePrintReadyDocument();

  PdfMetafileSkia* metafile = print_preview_context_.metafile();

  PrintHostMsg_DidPreviewDocument_Params preview_params;

  // Ask the browser to create the shared memory for us.
  if (!CopyMetafileDataToSharedMem(*metafile,
                                   &(preview_params.metafile_data_handle))) {
    print_preview_context_.set_error(PREVIEW_ERROR_METAFILE_COPY_FAILED);
    return false;
  }

  preview_params.data_size = metafile->GetDataSize();
  preview_params.document_cookie = print_pages_params_->params.document_cookie;
  preview_params.expected_pages_count =
      print_preview_context_.total_page_count();
  preview_params.modifiable = print_preview_context_.IsModifiable();
  preview_params.preview_request_id =
      print_pages_params_->params.preview_request_id;

  is_print_ready_metafile_sent_ = true;

  Send(new PrintHostMsg_MetafileReadyForPrinting(routing_id(), preview_params));
  return true;
}

void PrintWebViewHelper::PrintNode(const blink::WebNode& node) {
  if (node.IsNull() || !node.GetDocument().GetFrame()) {
    // This can occur when the context menu refers to an invalid WebNode.
    // See http://crbug.com/100890#c17 for a repro case.
    return;
  }

  if (print_node_in_progress_) {
    // This can happen as a result of processing sync messages when printing
    // from ppapi plugins. It's a rare case, so its OK to just fail here.
    // See http://crbug.com/159165.
    return;
  }

  print_node_in_progress_ = true;
  blink::WebNode duplicate_node(node);
  Print(duplicate_node.GetDocument().GetFrame(), duplicate_node);

  print_node_in_progress_ = false;
}

void PrintWebViewHelper::Print(blink::WebLocalFrame* frame,
                               const blink::WebNode& node,
                               bool silent,
                               bool print_background,
                               const base::string16& device_name) {
  // If still not finished with earlier print request simply ignore.
  if (prep_frame_view_)
    return;

  FrameReference frame_ref(frame);

  int expected_page_count = 0;
  if (!CalculateNumberOfPages(frame, node, &expected_page_count, device_name)) {
    DidFinishPrinting(FAIL_PRINT_INIT);
    return;  // Failed to init print page settings.
  }

  // Some full screen plugins can say they don't want to print.
  if (!expected_page_count) {
    DidFinishPrinting(FAIL_PRINT);
    return;
  }

  // Ask the browser to show UI to retrieve the final print settings.
  if (!silent && !GetPrintSettingsFromUser(frame_ref.GetFrame(), node,
                                           expected_page_count)) {
    DidFinishPrinting(OK);  // Release resources and fail silently.
    return;
  }

  print_pages_params_->params.should_print_backgrounds = print_background;

  // Render Pages for printing.
  if (!RenderPagesForPrint(frame_ref.GetFrame(), node)) {
    DidFinishPrinting(FAIL_PRINT);
  }
}

void PrintWebViewHelper::DidFinishPrinting(PrintingResult result) {
  switch (result) {
    case OK:
      break;

    case FAIL_PRINT_INIT:
      DCHECK(!notify_browser_of_print_failure_);
      break;

    case FAIL_PRINT:
      if (notify_browser_of_print_failure_ && print_pages_params_) {
        int cookie = print_pages_params_->params.document_cookie;
        Send(new PrintHostMsg_PrintingFailed(routing_id(), cookie));
      }
      break;

    case FAIL_PREVIEW:
    case INVALID_SETTINGS:
      if (print_pages_params_) {
        Send(new PrintHostMsg_PrintPreviewFailed(
            routing_id(), print_pages_params_->params.document_cookie,
            print_pages_params_->params.preview_request_id));
      }
      break;
  }
  prep_frame_view_.reset();
  print_pages_params_.reset();
  notify_browser_of_print_failure_ = true;
}

void PrintWebViewHelper::OnFramePreparedForPrintPages() {
  PrintPages();
  FinishFramePrinting();
}

void PrintWebViewHelper::PrintPages() {
  if (!prep_frame_view_)  // Printing is already canceled or failed.
    return;
  prep_frame_view_->StartPrinting();

  int page_count = prep_frame_view_->GetExpectedPageCount();
  if (!page_count) {
    LOG(ERROR) << "Can't print 0 pages.";
    return DidFinishPrinting(FAIL_PRINT);
  }

  const PrintMsg_PrintPages_Params& params = *print_pages_params_;
  const PrintMsg_Print_Params& print_params = params.params;

#if !defined(OS_CHROMEOS) && !defined(OS_ANDROID)
  // TODO(vitalybuka): should be page_count or valid pages from params.pages.
  // See http://crbug.com/161576
  Send(new PrintHostMsg_DidGetPrintedPagesCount(
      routing_id(), print_params.document_cookie, page_count));
#endif  // !defined(OS_CHROMEOS)

  if (!PrintPagesNative(prep_frame_view_->frame(), page_count)) {
    LOG(ERROR) << "Printing failed.";
    return DidFinishPrinting(FAIL_PRINT);
  }
}

void PrintWebViewHelper::FinishFramePrinting() {
  prep_frame_view_.reset();
}

#if defined(OS_MACOSX)
bool PrintWebViewHelper::PrintPagesNative(blink::WebLocalFrame* frame,
                                          int page_count) {
  const PrintMsg_PrintPages_Params& params = *print_pages_params_;
  const PrintMsg_Print_Params& print_params = params.params;

  PrintMsg_PrintPage_Params page_params;
  page_params.params = print_params;
  if (params.pages.empty()) {
    for (int i = 0; i < page_count; ++i) {
      page_params.page_number = i;
      PrintPageInternal(page_params, frame);
    }
  } else {
    for (size_t i = 0; i < params.pages.size(); ++i) {
      if (params.pages[i] >= page_count)
        break;
      page_params.page_number = params.pages[i];
      PrintPageInternal(page_params, frame);
    }
  }
  return true;
}

#endif  // OS_MACOSX

// static - Not anonymous so that platform implementations can use it.
void PrintWebViewHelper::ComputePageLayoutInPointsForCss(
    blink::WebLocalFrame* frame,
    int page_index,
    const PrintMsg_Print_Params& page_params,
    bool ignore_css_margins,
    double* scale_factor,
    PageSizeMargins* page_layout_in_points) {
  PrintMsg_Print_Params params = CalculatePrintParamsForCss(
      frame, page_index, page_params, ignore_css_margins,
      page_params.print_scaling_option ==
          blink::kWebPrintScalingOptionFitToPrintableArea,
      scale_factor);
  CalculatePageLayoutFromPrintParams(params, page_layout_in_points);
}

bool PrintWebViewHelper::InitPrintSettings(bool fit_to_paper_size,
                                           const base::string16& device_name) {
  PrintMsg_PrintPages_Params settings;
  if (device_name.empty()) {
    Send(new PrintHostMsg_GetDefaultPrintSettings(routing_id(),
                                                  &settings.params));
  } else {
    Send(new PrintHostMsg_InitSettingWithDeviceName(routing_id(), device_name,
                                                    &settings.params));
  }
  // Check if the printer returned any settings, if the settings is empty, we
  // can safely assume there are no printer drivers configured. So we safely
  // terminate.
  bool result = true;
  if (!PrintMsg_Print_Params_IsValid(settings.params))
    result = false;

  // Reset to default values.
  ignore_css_margins_ = false;
  settings.pages.clear();

  settings.params.print_scaling_option =
      blink::kWebPrintScalingOptionSourceSize;
  if (fit_to_paper_size) {
    settings.params.print_scaling_option =
        blink::kWebPrintScalingOptionFitToPrintableArea;
  }

  SetPrintPagesParams(settings);
  return result;
}

bool PrintWebViewHelper::CalculateNumberOfPages(
    blink::WebLocalFrame* frame,
    const blink::WebNode& node,
    int* number_of_pages,
    const base::string16& device_name) {
  DCHECK(frame);
  bool fit_to_paper_size = !(PrintingNodeOrPdfFrame(frame, node));
  if (!InitPrintSettings(fit_to_paper_size, device_name)) {
    notify_browser_of_print_failure_ = false;
    Send(new PrintHostMsg_ShowInvalidPrinterSettingsError(routing_id()));
    return false;
  }

  const PrintMsg_Print_Params& params = print_pages_params_->params;
  PrepareFrameAndViewForPrint prepare(params, frame, node, ignore_css_margins_);
  prepare.StartPrinting();

  *number_of_pages = prepare.GetExpectedPageCount();
  return true;
}

bool PrintWebViewHelper::UpdatePrintSettings(
    blink::WebLocalFrame* frame,
    const blink::WebNode& node,
    const base::DictionaryValue& passed_job_settings) {
  const base::DictionaryValue* job_settings = &passed_job_settings;
  base::DictionaryValue modified_job_settings;
  if (job_settings->empty()) {
    if (!print_for_preview_)
      print_preview_context_.set_error(PREVIEW_ERROR_BAD_SETTING);
    return false;
  }

  bool source_is_html = true;
  if (print_for_preview_) {
    if (!job_settings->GetBoolean(kSettingPreviewModifiable, &source_is_html)) {
      NOTREACHED();
    }
  } else {
    source_is_html = !PrintingNodeOrPdfFrame(frame, node);
  }

  if (print_for_preview_ || !source_is_html) {
    modified_job_settings.MergeDictionary(job_settings);
    modified_job_settings.SetBoolean(kSettingHeaderFooterEnabled, false);
    modified_job_settings.SetInteger(kSettingMarginsType, NO_MARGINS);
    job_settings = &modified_job_settings;
  }

  // Send the cookie so that UpdatePrintSettings can reuse PrinterQuery when
  // possible.
  int cookie =
      print_pages_params_ ? print_pages_params_->params.document_cookie : 0;
  PrintMsg_PrintPages_Params settings;
  bool canceled = false;
  Send(new PrintHostMsg_UpdatePrintSettings(routing_id(), cookie, *job_settings,
                                            &settings, &canceled));
  if (canceled) {
    notify_browser_of_print_failure_ = false;
    return false;
  }

  if (!print_for_preview_) {
    job_settings->GetInteger(kPreviewRequestID,
                             &settings.params.preview_request_id);
    settings.params.print_to_pdf = true;
    UpdateFrameMarginsCssInfo(*job_settings);
    settings.params.print_scaling_option =
        blink::kWebPrintScalingOptionSourceSize;
  }

  SetPrintPagesParams(settings);

  if (PrintMsg_Print_Params_IsValid(settings.params))
    return true;

  if (print_for_preview_)
    Send(new PrintHostMsg_ShowInvalidPrinterSettingsError(routing_id()));
  else
    print_preview_context_.set_error(PREVIEW_ERROR_INVALID_PRINTER_SETTINGS);
  return false;
}

bool PrintWebViewHelper::GetPrintSettingsFromUser(blink::WebLocalFrame* frame,
                                                  const blink::WebNode& node,
                                                  int expected_pages_count) {
  PrintHostMsg_ScriptedPrint_Params params;
  PrintMsg_PrintPages_Params print_settings;

  params.cookie = print_pages_params_->params.document_cookie;
  params.has_selection = frame->HasSelection();
  params.expected_pages_count = expected_pages_count;
  MarginType margin_type = DEFAULT_MARGINS;
  if (PrintingNodeOrPdfFrame(frame, node))
    margin_type = GetMarginsForPdf(frame, node);
  params.margin_type = margin_type;

  // PrintHostMsg_ScriptedPrint will reset print_scaling_option, so we save the
  // value before and restore it afterwards.
  blink::WebPrintScalingOption scaling_option =
      print_pages_params_->params.print_scaling_option;

  print_pages_params_.reset();
  IPC::SyncMessage* msg =
      new PrintHostMsg_ScriptedPrint(routing_id(), params, &print_settings);
  msg->EnableMessagePumping();
  Send(msg);
  print_settings.params.print_scaling_option = scaling_option;
  SetPrintPagesParams(print_settings);
  return (print_settings.params.dpi && print_settings.params.document_cookie);
}

bool PrintWebViewHelper::RenderPagesForPrint(blink::WebLocalFrame* frame,
                                             const blink::WebNode& node) {
  if (!frame || prep_frame_view_)
    return false;
  const PrintMsg_PrintPages_Params& params = *print_pages_params_;
  const PrintMsg_Print_Params& print_params = params.params;
  prep_frame_view_.reset(new PrepareFrameAndViewForPrint(
      print_params, frame, node, ignore_css_margins_));
  DCHECK(!print_pages_params_->params.selection_only ||
         print_pages_params_->pages.empty());
  prep_frame_view_->CopySelectionIfNeeded(
      render_frame()->GetWebkitPreferences(),
      base::Bind(&PrintWebViewHelper::OnFramePreparedForPrintPages,
                 base::Unretained(this)));
  return true;
}

#if defined(OS_POSIX)
bool PrintWebViewHelper::CopyMetafileDataToSharedMem(
    const PdfMetafileSkia& metafile,
    base::SharedMemoryHandle* shared_mem_handle) {
  uint32_t buf_size = metafile.GetDataSize();
  if (buf_size == 0)
    return false;

  std::unique_ptr<base::SharedMemory> shared_buf(
      content::RenderThread::Get()->HostAllocateSharedMemoryBuffer(buf_size));
  if (!shared_buf)
    return false;

  if (!shared_buf->Map(buf_size))
    return false;

  if (!metafile.GetData(shared_buf->memory(), buf_size))
    return false;

  *shared_mem_handle =
      base::SharedMemory::DuplicateHandle(shared_buf->handle());
  return true;
}
#endif  // defined(OS_POSIX)

void PrintWebViewHelper::SetPrintPagesParams(
    const PrintMsg_PrintPages_Params& settings) {
  print_pages_params_.reset(new PrintMsg_PrintPages_Params(settings));
  Send(new PrintHostMsg_DidGetDocumentCookie(routing_id(),
                                             settings.params.document_cookie));
}

bool PrintWebViewHelper::PreviewPageRendered(int page_number,
                                             PdfMetafileSkia* metafile) {
  DCHECK_GE(page_number, FIRST_PAGE_INDEX);

  // For non-modifiable files, |metafile| should be NULL, so do not bother
  // sending a message. If we don't generate draft metafiles, |metafile| is
  // NULL.
  if (!print_preview_context_.IsModifiable() ||
      !print_preview_context_.generate_draft_pages()) {
    DCHECK(!metafile);
    return true;
  }

  if (!metafile) {
    NOTREACHED();
    print_preview_context_.set_error(
        PREVIEW_ERROR_PAGE_RENDERED_WITHOUT_METAFILE);
    return false;
  }

  return true;
}

PrintWebViewHelper::PrintPreviewContext::PrintPreviewContext()
    : total_page_count_(0),
      current_page_index_(0),
      generate_draft_pages_(true),
      print_ready_metafile_page_count_(0),
      error_(PREVIEW_ERROR_NONE),
      state_(UNINITIALIZED) {}

PrintWebViewHelper::PrintPreviewContext::~PrintPreviewContext() {}

void PrintWebViewHelper::PrintPreviewContext::InitWithFrame(
    blink::WebLocalFrame* web_frame) {
  DCHECK(web_frame);
  DCHECK(!IsRendering());
  state_ = INITIALIZED;
  source_frame_.Reset(web_frame);
  source_node_.Reset();
}

void PrintWebViewHelper::PrintPreviewContext::InitWithNode(
    const blink::WebNode& web_node) {
  DCHECK(!web_node.IsNull());
  DCHECK(web_node.GetDocument().GetFrame());
  DCHECK(!IsRendering());
  state_ = INITIALIZED;
  source_frame_.Reset(web_node.GetDocument().GetFrame());
  source_node_ = web_node;
}

void PrintWebViewHelper::PrintPreviewContext::OnPrintPreview() {
  DCHECK_EQ(INITIALIZED, state_);
  ClearContext();
}

bool PrintWebViewHelper::PrintPreviewContext::CreatePreviewDocument(
    PrepareFrameAndViewForPrint* prepared_frame,
    const std::vector<int>& pages) {
  DCHECK_EQ(INITIALIZED, state_);
  state_ = RENDERING;

  // Need to make sure old object gets destroyed first.
  prep_frame_view_.reset(prepared_frame);
  prep_frame_view_->StartPrinting();

  total_page_count_ = prep_frame_view_->GetExpectedPageCount();
  if (total_page_count_ == 0) {
    LOG(ERROR) << "CreatePreviewDocument got 0 page count";
    set_error(PREVIEW_ERROR_ZERO_PAGES);
    return false;
  }

  metafile_.reset(new PdfMetafileSkia());
  CHECK(metafile_->Init());

  current_page_index_ = 0;
  pages_to_render_ = pages;
  // Sort and make unique.
  std::sort(pages_to_render_.begin(), pages_to_render_.end());
  pages_to_render_.resize(
      std::unique(pages_to_render_.begin(), pages_to_render_.end()) -
      pages_to_render_.begin());
  // Remove invalid pages.
  pages_to_render_.resize(std::lower_bound(pages_to_render_.begin(),
                                           pages_to_render_.end(),
                                           total_page_count_) -
                          pages_to_render_.begin());
  print_ready_metafile_page_count_ = pages_to_render_.size();
  if (pages_to_render_.empty()) {
    print_ready_metafile_page_count_ = total_page_count_;
    // Render all pages.
    for (int i = 0; i < total_page_count_; ++i)
      pages_to_render_.push_back(i);
  } else if (generate_draft_pages_) {
    int pages_index = 0;
    for (int i = 0; i < total_page_count_; ++i) {
      if (pages_index < print_ready_metafile_page_count_ &&
          i == pages_to_render_[pages_index]) {
        pages_index++;
        continue;
      }
      pages_to_render_.push_back(i);
    }
  }

  document_render_time_ = base::TimeDelta();
  begin_time_ = base::TimeTicks::Now();

  return true;
}

void PrintWebViewHelper::PrintPreviewContext::RenderedPreviewPage(
    const base::TimeDelta& page_time) {
  DCHECK_EQ(RENDERING, state_);
  document_render_time_ += page_time;
  UMA_HISTOGRAM_TIMES("PrintPreview.RenderPDFPageTime", page_time);
}

void PrintWebViewHelper::PrintPreviewContext::AllPagesRendered() {
  DCHECK_EQ(RENDERING, state_);
  state_ = DONE;
  prep_frame_view_->FinishPrinting();
}

void PrintWebViewHelper::PrintPreviewContext::FinalizePrintReadyDocument() {
  DCHECK(IsRendering());

  base::TimeTicks begin_time = base::TimeTicks::Now();
  metafile_->FinishDocument();

  if (print_ready_metafile_page_count_ <= 0) {
    NOTREACHED();
    return;
  }

  UMA_HISTOGRAM_MEDIUM_TIMES("PrintPreview.RenderToPDFTime",
                             document_render_time_);
  base::TimeDelta total_time =
      (base::TimeTicks::Now() - begin_time) + document_render_time_;
  UMA_HISTOGRAM_MEDIUM_TIMES("PrintPreview.RenderAndGeneratePDFTime",
                             total_time);
  UMA_HISTOGRAM_MEDIUM_TIMES("PrintPreview.RenderAndGeneratePDFTimeAvgPerPage",
                             total_time / pages_to_render_.size());
}

void PrintWebViewHelper::PrintPreviewContext::Finished() {
  DCHECK_EQ(DONE, state_);
  state_ = INITIALIZED;
  ClearContext();
}

void PrintWebViewHelper::PrintPreviewContext::Failed(bool report_error) {
  DCHECK(state_ == INITIALIZED || state_ == RENDERING);
  state_ = INITIALIZED;
  if (report_error) {
    DCHECK_NE(PREVIEW_ERROR_NONE, error_);
    UMA_HISTOGRAM_ENUMERATION("PrintPreview.RendererError", error_,
                              PREVIEW_ERROR_LAST_ENUM);
  }
  ClearContext();
}

int PrintWebViewHelper::PrintPreviewContext::GetNextPageNumber() {
  DCHECK_EQ(RENDERING, state_);
  if (IsFinalPageRendered())
    return -1;
  return pages_to_render_[current_page_index_++];
}

bool PrintWebViewHelper::PrintPreviewContext::IsRendering() const {
  return state_ == RENDERING || state_ == DONE;
}

bool PrintWebViewHelper::PrintPreviewContext::IsModifiable() {
  // The only kind of node we can print right now is a PDF node.
  return !PrintingNodeOrPdfFrame(source_frame(), source_node_);
}

bool PrintWebViewHelper::PrintPreviewContext::HasSelection() {
  return IsModifiable() && source_frame()->HasSelection();
}

bool PrintWebViewHelper::PrintPreviewContext::IsLastPageOfPrintReadyMetafile()
    const {
  DCHECK(IsRendering());
  return current_page_index_ == print_ready_metafile_page_count_;
}

bool PrintWebViewHelper::PrintPreviewContext::IsFinalPageRendered() const {
  DCHECK(IsRendering());
  return static_cast<size_t>(current_page_index_) == pages_to_render_.size();
}

void PrintWebViewHelper::PrintPreviewContext::set_generate_draft_pages(
    bool generate_draft_pages) {
  DCHECK_EQ(INITIALIZED, state_);
  generate_draft_pages_ = generate_draft_pages;
}

void PrintWebViewHelper::PrintPreviewContext::set_error(
    enum PrintPreviewErrorBuckets error) {
  error_ = error;
}

blink::WebLocalFrame* PrintWebViewHelper::PrintPreviewContext::source_frame() {
  DCHECK(state_ != UNINITIALIZED);
  return source_frame_.GetFrame();
}

const blink::WebNode& PrintWebViewHelper::PrintPreviewContext::source_node()
    const {
  DCHECK(state_ != UNINITIALIZED);
  return source_node_;
}

blink::WebLocalFrame*
PrintWebViewHelper::PrintPreviewContext::prepared_frame() {
  DCHECK(state_ != UNINITIALIZED);
  return prep_frame_view_->frame();
}

const blink::WebNode& PrintWebViewHelper::PrintPreviewContext::prepared_node()
    const {
  DCHECK(state_ != UNINITIALIZED);
  return prep_frame_view_->node();
}

int PrintWebViewHelper::PrintPreviewContext::total_page_count() const {
  DCHECK(state_ != UNINITIALIZED);
  return total_page_count_;
}

bool PrintWebViewHelper::PrintPreviewContext::generate_draft_pages() const {
  return generate_draft_pages_;
}

PdfMetafileSkia* PrintWebViewHelper::PrintPreviewContext::metafile() {
  DCHECK(IsRendering());
  return metafile_.get();
}

int PrintWebViewHelper::PrintPreviewContext::last_error() const {
  return error_;
}

void PrintWebViewHelper::PrintPreviewContext::ClearContext() {
  prep_frame_view_.reset();
  metafile_.reset();
  pages_to_render_.clear();
  error_ = PREVIEW_ERROR_NONE;
}

}  // namespace printing
