// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/printing/print_web_view_helper.h"

#import <AppKit/AppKit.h>

#include "base/logging.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/metrics/histogram.h"
#include "cc/paint/paint_canvas.h"
#include "chrome/common/print_messages.h"
#include "printing/metafile_skia_wrapper.h"
#include "printing/page_size_margins.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace printing {

using blink::WebLocalFrame;

void PrintWebViewHelper::PrintPageInternal(
    const PrintMsg_PrintPage_Params& params,
    WebLocalFrame* frame) {
  PdfMetafileSkia metafile;
  CHECK(metafile.Init());

  int page_number = params.page_number;
  gfx::Size page_size_in_dpi;
  gfx::Rect content_area_in_dpi;
  RenderPage(print_pages_params_->params, page_number, frame, &metafile,
             &page_size_in_dpi, &content_area_in_dpi);
  metafile.FinishDocument();

  PrintHostMsg_DidPrintPage_Params page_params;
  page_params.data_size = metafile.GetDataSize();
  page_params.page_number = page_number;
  page_params.document_cookie = params.params.document_cookie;
  page_params.page_size = page_size_in_dpi;
  page_params.content_area = content_area_in_dpi;

  // Ask the browser to create the shared memory for us.
  if (!CopyMetafileDataToSharedMem(metafile,
                                   &(page_params.metafile_data_handle))) {
    // TODO(thestig): Fail and return false instead.
    page_params.data_size = 0;
  }

  Send(new PrintHostMsg_DidPrintPage(routing_id(), page_params));
}

bool PrintWebViewHelper::RenderPreviewPage(
    int page_number,
    const PrintMsg_Print_Params& print_params) {
  PrintMsg_Print_Params printParams = print_params;
  std::unique_ptr<PdfMetafileSkia> draft_metafile;
  PdfMetafileSkia* initial_render_metafile = print_preview_context_.metafile();

  bool render_to_draft =
      print_preview_context_.IsModifiable() && is_print_ready_metafile_sent_;

  if (render_to_draft) {
    draft_metafile.reset(new PdfMetafileSkia());
    CHECK(draft_metafile->Init());
    initial_render_metafile = draft_metafile.get();
  }

  base::TimeTicks begin_time = base::TimeTicks::Now();
  gfx::Size page_size;
  RenderPage(printParams, page_number, print_preview_context_.prepared_frame(),
             initial_render_metafile, &page_size, nullptr);
  print_preview_context_.RenderedPreviewPage(base::TimeTicks::Now() -
                                             begin_time);

  if (draft_metafile.get()) {
    draft_metafile->FinishDocument();
  } else {
    if (print_preview_context_.IsModifiable() &&
        print_preview_context_.generate_draft_pages()) {
      DCHECK(!draft_metafile.get());
      draft_metafile =
          print_preview_context_.metafile()->GetMetafileForCurrentPage(
              SkiaDocumentType::PDF);
    }
  }
  return PreviewPageRendered(page_number, draft_metafile.get());
}

void PrintWebViewHelper::RenderPage(const PrintMsg_Print_Params& params,
                                    int page_number,
                                    WebLocalFrame* frame,
                                    PdfMetafileSkia* metafile,
                                    gfx::Size* page_size,
                                    gfx::Rect* content_rect) {
  double scale_factor = 1.0f;
  double webkit_shrink_factor = frame->GetPrintPageShrink(page_number);
  PageSizeMargins page_layout_in_points;
  gfx::Rect content_area;

  ComputePageLayoutInPointsForCss(frame, page_number, params,
                                  ignore_css_margins_, &scale_factor,
                                  &page_layout_in_points);
  GetPageSizeAndContentAreaFromPageLayout(page_layout_in_points, page_size,
                                          &content_area);
  if (content_rect)
    *content_rect = content_area;

  scale_factor *= webkit_shrink_factor;

  gfx::Rect canvas_area = content_area;

  {
    cc::PaintCanvas* canvas = metafile->GetVectorCanvasForNewPage(
        *page_size, canvas_area, scale_factor);
    if (!canvas)
      return;

    MetafileSkiaWrapper::SetMetafileOnCanvas(canvas, metafile);
    RenderPageContent(frame, page_number, canvas_area, content_area,
                      scale_factor, static_cast<cc::PaintCanvas*>(canvas));
  }

  // Done printing. Close the device context to retrieve the compiled metafile.
  metafile->FinishPage();
}

}  // namespace printing
