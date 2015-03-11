// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/printing/print_web_view_helper.h"

#import <AppKit/AppKit.h>

#include "base/logging.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/metrics/histogram.h"
#include "chrome/common/print_messages.h"
#include "printing/metafile_skia_wrapper.h"
#include "printing/page_size_margins.h"
#include "skia/ext/platform_device.h"
#include "skia/ext/vector_canvas.h"
#include "third_party/WebKit/public/platform/WebCanvas.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace printing {

using blink::WebFrame;

void PrintWebViewHelper::PrintPageInternal(
    const PrintMsg_PrintPage_Params& params,
    WebFrame* frame) {
  PdfMetafileSkia metafile;
  if (!metafile.Init())
    return;

  int page_number = params.page_number;
  gfx::Size page_size_in_dpi;
  gfx::Rect content_area_in_dpi;
  RenderPage(print_pages_params_->params, page_number, frame, false, &metafile,
             &page_size_in_dpi, &content_area_in_dpi);
  metafile.FinishDocument();

  PrintHostMsg_DidPrintPage_Params page_params;
  page_params.data_size = metafile.GetDataSize();
  page_params.page_number = page_number;
  page_params.document_cookie = params.params.document_cookie;
  page_params.page_size = page_size_in_dpi;
  page_params.content_area = content_area_in_dpi;

  // Ask the browser to create the shared memory for us.
  if (!CopyMetafileDataToSharedMem(&metafile,
                                   &(page_params.metafile_data_handle))) {
    page_params.data_size = 0;
  }

  Send(new PrintHostMsg_DidPrintPage(routing_id(), page_params));
}

void PrintWebViewHelper::RenderPage(const PrintMsg_Print_Params& params,
                                    int page_number,
                                    WebFrame* frame,
                                    bool is_preview,
                                    PdfMetafileSkia* metafile,
                                    gfx::Size* page_size,
                                    gfx::Rect* content_rect) {
  double scale_factor = 1.0f;
  double webkit_shrink_factor = frame->getPrintPageShrink(page_number);
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
    skia::PlatformCanvas* canvas = metafile->GetVectorCanvasForNewPage(
        *page_size, canvas_area, scale_factor);
    if (!canvas)
      return;

    MetafileSkiaWrapper::SetMetafileOnCanvas(*canvas, metafile);
    skia::SetIsDraftMode(*canvas, is_print_ready_metafile_sent_);
    skia::SetIsPreviewMetafile(*canvas, is_preview);

    RenderPageContent(frame, page_number, canvas_area, content_area,
                      scale_factor, static_cast<blink::WebCanvas*>(canvas));
  }

  // Done printing. Close the device context to retrieve the compiled metafile.
  metafile->FinishPage();
}

}  // namespace printing
