// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PRINTING_PRINT_WEB_VIEW_HELPER_H_
#define CHROME_RENDERER_PRINTING_PRINT_WEB_VIEW_HELPER_H_

#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/shared_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "content/public/renderer/render_view_observer.h"
#include "content/public/renderer/render_view_observer_tracker.h"
#include "printing/metafile_impl.h"
#include "third_party/WebKit/public/platform/WebCanvas.h"
#include "third_party/WebKit/public/web/WebNode.h"
#include "third_party/WebKit/public/web/WebPrintParams.h"
#include "ui/gfx/size.h"

struct PrintMsg_Print_Params;
struct PrintMsg_PrintPage_Params;
struct PrintMsg_PrintPages_Params;

namespace base {
class DictionaryValue;
}

namespace blink {
class WebFrame;
class WebView;
}

namespace printing {

struct PageSizeMargins;
class PrepareFrameAndViewForPrint;

// Stores reference to frame using WebVew and unique name.
// Workaround to modal dialog issue on Linux. crbug.com/236147.
// If WebFrame someday supports WeakPtr, we should use it here.
class FrameReference {
 public:
  explicit FrameReference(blink::WebLocalFrame* frame);
  FrameReference();
  ~FrameReference();

  void Reset(blink::WebLocalFrame* frame);

  blink::WebLocalFrame* GetFrame();
  blink::WebView* view();

 private:
  blink::WebView* view_;
  blink::WebLocalFrame* frame_;
};

// PrintWebViewHelper handles most of the printing grunt work for RenderView.
// We plan on making print asynchronous and that will require copying the DOM
// of the document and creating a new WebView with the contents.
class PrintWebViewHelper
    : public content::RenderViewObserver,
      public content::RenderViewObserverTracker<PrintWebViewHelper> {
 public:
  explicit PrintWebViewHelper(content::RenderView* render_view);
  virtual ~PrintWebViewHelper();

  bool IsPrintingEnabled();

  void PrintNode(const blink::WebNode& node);

 private:
  friend class PrintWebViewHelperTestBase;
  FRIEND_TEST_ALL_PREFIXES(PrintWebViewHelperTest,
                           BlockScriptInitiatedPrinting);
  FRIEND_TEST_ALL_PREFIXES(PrintWebViewHelperTest,
                           BlockScriptInitiatedPrintingFromPopup);
  FRIEND_TEST_ALL_PREFIXES(PrintWebViewHelperTest, OnPrintPages);

#if defined(OS_WIN) || defined(OS_MACOSX)
  FRIEND_TEST_ALL_PREFIXES(PrintWebViewHelperTest, PrintLayoutTest);
  FRIEND_TEST_ALL_PREFIXES(PrintWebViewHelperTest, PrintWithIframe);
#endif  // defined(OS_WIN) || defined(OS_MACOSX)

  enum PrintingResult {
    OK,
    FAIL_PRINT_INIT,
    FAIL_PRINT,
  };

  // RenderViewObserver implementation.
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;
  virtual void PrintPage(blink::WebLocalFrame* frame,
                         bool user_initiated) OVERRIDE;
  virtual void DidStartLoading() OVERRIDE;
  virtual void DidStopLoading() OVERRIDE;

  // Message handlers ---------------------------------------------------------
  void OnPrintPages();
  void OnPrintForSystemDialog();
  void OnPrintingDone(bool success);

  // Get |page_size| and |content_area| information from
  // |page_layout_in_points|.
  void GetPageSizeAndContentAreaFromPageLayout(
      const PageSizeMargins& page_layout_in_points,
      gfx::Size* page_size,
      gfx::Rect* content_area);

  // Update |ignore_css_margins_| based on settings.
  void UpdateFrameMarginsCssInfo(const base::DictionaryValue& settings);

  // Returns true if the current destination printer is PRINT_TO_PDF.
  bool IsPrintToPdfRequested(const base::DictionaryValue& settings);

  // Enable/Disable window.print calls.  If |blocked| is true window.print
  // calls will silently fail.  Call with |blocked| set to false to reenable.
  void SetScriptedPrintBlocked(bool blocked);

  // Main printing code -------------------------------------------------------

  void Print(blink::WebLocalFrame* frame, const blink::WebNode& node);

  // Notification when printing is done - signal tear-down/free resources.
  void DidFinishPrinting(PrintingResult result);

  // Print Settings -----------------------------------------------------------

  // Initialize print page settings with default settings.
  // Used only for native printing workflow.
  bool InitPrintSettings(bool fit_to_paper_size);

  // Calculate number of pages in source document.
  bool CalculateNumberOfPages(blink::WebLocalFrame* frame,
                              const blink::WebNode& node,
                              int* number_of_pages);

  // Get final print settings from the user.
  // Return false if the user cancels or on error.
  bool GetPrintSettingsFromUser(blink::WebFrame* frame,
                                const blink::WebNode& node,
                                int expected_pages_count);

  // Page Printing / Rendering ------------------------------------------------

  void OnFramePreparedForPrintPages();
  void PrintPages();
  bool PrintPagesNative(blink::WebFrame* frame,
                        int page_count,
                        const gfx::Size& canvas_size);
  void FinishFramePrinting();

  // Prints the page listed in |params|.
#if defined(OS_LINUX) || defined(OS_ANDROID)
  void PrintPageInternal(const PrintMsg_PrintPage_Params& params,
                         const gfx::Size& canvas_size,
                         blink::WebFrame* frame,
                         Metafile* metafile);
#else
  void PrintPageInternal(const PrintMsg_PrintPage_Params& params,
                         const gfx::Size& canvas_size,
                         blink::WebFrame* frame);
#endif

  // Render the frame for printing.
  bool RenderPagesForPrint(blink::WebLocalFrame* frame,
                           const blink::WebNode& node);

  // Platform specific helper function for rendering page(s) to |metafile|.
#if defined(OS_WIN)
  void RenderPage(const PrintMsg_Print_Params& params,
                  int page_number,
                  blink::WebFrame* frame,
                  bool is_preview,
                  Metafile* metafile,
                  double* scale_factor,
                  gfx::Size* page_size_in_dpi,
                  gfx::Rect* content_area_in_dpi);
#elif defined(OS_MACOSX)
  void RenderPage(const PrintMsg_Print_Params& params,
                  int page_number,
                  blink::WebFrame* frame,
                  bool is_preview,
                  Metafile* metafile,
                  gfx::Size* page_size,
                  gfx::Rect* content_rect);
#endif  // defined(OS_WIN)

  // Renders page contents from |frame| to |content_area| of |canvas|.
  // |page_number| is zero-based.
  // When method is called, canvas should be setup to draw to |canvas_area|
  // with |scale_factor|.
  static float RenderPageContent(blink::WebFrame* frame,
                                 int page_number,
                                 const gfx::Rect& canvas_area,
                                 const gfx::Rect& content_area,
                                 double scale_factor,
                                 blink::WebCanvas* canvas);

  // Helper methods -----------------------------------------------------------

  bool CopyMetafileDataToSharedMem(Metafile* metafile,
                                   base::SharedMemoryHandle* shared_mem_handle);

  // Helper method to get page layout in points and fit to page if needed.
  static void ComputePageLayoutInPointsForCss(
      blink::WebFrame* frame,
      int page_index,
      const PrintMsg_Print_Params& default_params,
      bool ignore_css_margins,
      double* scale_factor,
      PageSizeMargins* page_layout_in_points);

  // Given the |device| and |canvas| to draw on, prints the appropriate headers
  // and footers using strings from |header_footer_info| on to the canvas.
  static void PrintHeaderAndFooter(
      blink::WebCanvas* canvas,
      int page_number,
      int total_pages,
      float webkit_scale_factor,
      const PageSizeMargins& page_layout_in_points,
      const base::DictionaryValue& header_footer_info,
      const PrintMsg_Print_Params& params);

  bool GetPrintFrame(blink::WebLocalFrame** frame);

  // Script Initiated Printing ------------------------------------------------

  // WebView used only to print the selection.
  scoped_ptr<PrepareFrameAndViewForPrint> prep_frame_view_;
  bool reset_prep_frame_view_;

  scoped_ptr<PrintMsg_PrintPages_Params> print_pages_params_;
  bool is_preview_enabled_;
  bool is_scripted_print_throttling_disabled_;
  bool is_print_ready_metafile_sent_;
  bool ignore_css_margins_;

  // Let the browser process know of a printing failure. Only set to false when
  // the failure came from the browser in the first place.
  bool notify_browser_of_print_failure_;

  // Strings generated by the browser process to be printed as headers and
  // footers if requested by the user.
  scoped_ptr<base::DictionaryValue> header_footer_info_;

  bool print_node_in_progress_;
  bool is_loading_;
  bool is_scripted_preview_delayed_;
  base::WeakPtrFactory<PrintWebViewHelper> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(PrintWebViewHelper);
};

}  // namespace printing

#endif  // CHROME_RENDERER_PRINTING_PRINT_WEB_VIEW_HELPER_H_
