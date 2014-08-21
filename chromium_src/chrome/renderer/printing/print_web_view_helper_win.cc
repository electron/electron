// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/printing/print_web_view_helper.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/metrics/histogram.h"
#include "base/process/process_handle.h"
#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "base/win/scoped_select_object.h"
#include "chrome/common/print_messages.h"
#include "printing/metafile.h"
#include "printing/metafile_impl.h"
#include "printing/metafile_skia_wrapper.h"
#include "printing/page_size_margins.h"
#include "printing/units.h"
#include "skia/ext/platform_device.h"
#include "skia/ext/refptr.h"
#include "skia/ext/vector_canvas.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "ui/gfx/gdi_util.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

namespace printing {

using blink::WebFrame;

void PrintWebViewHelper::PrintPageInternal(
    const PrintMsg_PrintPage_Params& params,
    const gfx::Size& canvas_size,
    WebFrame* frame) {
  // Generate a memory-based metafile. It will use the current screen's DPI.
  // Each metafile contains a single page.
  scoped_ptr<NativeMetafile> metafile(new NativeMetafile);
  metafile->Init();
  DCHECK(metafile->context());
  skia::InitializeDC(metafile->context());

  int page_number = params.page_number;

  // Calculate the dpi adjustment.
  // Browser will render context using desired_dpi, so we need to calculate
  // adjustment factor to play content on the printer DC later during the
  // actual printing.
  double actual_shrink = static_cast<float>(params.params.desired_dpi /
                                            params.params.dpi);
  gfx::Size page_size_in_dpi;
  gfx::Rect content_area_in_dpi;

  // Render page for printing.
  RenderPage(params.params, page_number, frame, false, metafile.get(),
             &actual_shrink, &page_size_in_dpi, &content_area_in_dpi);

  // Close the device context to retrieve the compiled metafile.
  if (!metafile->FinishDocument())
    NOTREACHED();

  if (!params.params.supports_alpha_blend && metafile->IsAlphaBlendUsed()) {
    scoped_ptr<NativeMetafile> raster_metafile(
        metafile->RasterizeAlphaBlend());
    if (raster_metafile.get())
      metafile.swap(raster_metafile);
  }

  // Get the size of the compiled metafile.
  uint32 buf_size = metafile->GetDataSize();
  DCHECK_GT(buf_size, 128u);

  PrintHostMsg_DidPrintPage_Params page_params;
  page_params.data_size = buf_size;
  page_params.metafile_data_handle = NULL;
  page_params.page_number = page_number;
  page_params.document_cookie = params.params.document_cookie;
  page_params.actual_shrink = actual_shrink;
  page_params.page_size = page_size_in_dpi;
  page_params.content_area = content_area_in_dpi;

  if (!CopyMetafileDataToSharedMem(metafile.get(),
                                   &(page_params.metafile_data_handle))) {
    page_params.data_size = 0;
  }

  Send(new PrintHostMsg_DidPrintPage(routing_id(), page_params));
}

bool PrintWebViewHelper::RenderPreviewPage(
    int page_number,
    const PrintMsg_Print_Params& print_params) {
  // Calculate the dpi adjustment.
  double actual_shrink = static_cast<float>(print_params.desired_dpi /
                                            print_params.dpi);
  scoped_ptr<Metafile> draft_metafile;
  Metafile* initial_render_metafile = print_preview_context_.metafile();

  if (print_preview_context_.IsModifiable() && is_print_ready_metafile_sent_) {
    draft_metafile.reset(new PreviewMetafile);
    initial_render_metafile = draft_metafile.get();
  }

  base::TimeTicks begin_time = base::TimeTicks::Now();
  RenderPage(print_params, page_number, print_preview_context_.prepared_frame(),
             true, initial_render_metafile, &actual_shrink, NULL, NULL);
  print_preview_context_.RenderedPreviewPage(
      base::TimeTicks::Now() - begin_time);

  if (draft_metafile.get()) {
    draft_metafile->FinishDocument();
  } else if (print_preview_context_.IsModifiable() &&
             print_preview_context_.generate_draft_pages()) {
    DCHECK(!draft_metafile.get());
    draft_metafile.reset(
        print_preview_context_.metafile()->GetMetafileForCurrentPage());
  }
  return PreviewPageRendered(page_number, draft_metafile.get());
}

void PrintWebViewHelper::RenderPage(
    const PrintMsg_Print_Params& params, int page_number, WebFrame* frame,
    bool is_preview, Metafile* metafile, double* actual_shrink,
    gfx::Size* page_size_in_dpi, gfx::Rect* content_area_in_dpi) {
  PageSizeMargins page_layout_in_points;
  double css_scale_factor = 1.0f;
  ComputePageLayoutInPointsForCss(frame, page_number, params,
                                  ignore_css_margins_, &css_scale_factor,
                                  &page_layout_in_points);
  gfx::Size page_size;
  gfx::Rect content_area;
  GetPageSizeAndContentAreaFromPageLayout(page_layout_in_points, &page_size,
                                          &content_area);
  int dpi = static_cast<int>(params.dpi);
  // Calculate the actual page size and content area in dpi.
  if (page_size_in_dpi) {
    *page_size_in_dpi = gfx::Size(
        static_cast<int>(ConvertUnitDouble(page_size.width(), kPointsPerInch,
                                           dpi)),
        static_cast<int>(ConvertUnitDouble(page_size.height(), kPointsPerInch,
                                           dpi)));
  }

  if (content_area_in_dpi) {
    *content_area_in_dpi = gfx::Rect(
        static_cast<int>(ConvertUnitDouble(content_area.x(), kPointsPerInch,
                                           dpi)),
        static_cast<int>(ConvertUnitDouble(content_area.y(), kPointsPerInch,
                                           dpi)),
        static_cast<int>(ConvertUnitDouble(content_area.width(), kPointsPerInch,
                                           dpi)),
        static_cast<int>(ConvertUnitDouble(content_area.height(),
                                           kPointsPerInch, dpi)));
  }

  if (!is_preview) {
    // Since WebKit extends the page width depending on the magical scale factor
    // we make sure the canvas covers the worst case scenario (x2.0 currently).
    // PrintContext will then set the correct clipping region.
    page_size = gfx::Size(
        static_cast<int>(page_layout_in_points.content_width *
                         params.max_shrink),
        static_cast<int>(page_layout_in_points.content_height *
                         params.max_shrink));
  }

  float webkit_page_shrink_factor = frame->getPrintPageShrink(page_number);
  float scale_factor = css_scale_factor * webkit_page_shrink_factor;

  gfx::Rect canvas_area =
      params.display_header_footer ? gfx::Rect(page_size) : content_area;

  SkBaseDevice* device = metafile->StartPageForVectorCanvas(
      page_size, canvas_area, scale_factor);
  DCHECK(device);
  // The printPage method may take a reference to the canvas we pass down, so it
  // can't be a stack object.
  skia::RefPtr<skia::VectorCanvas> canvas =
      skia::AdoptRef(new skia::VectorCanvas(device));

  if (is_preview) {
    MetafileSkiaWrapper::SetMetafileOnCanvas(*canvas, metafile);
    skia::SetIsDraftMode(*canvas, is_print_ready_metafile_sent_);
    skia::SetIsPreviewMetafile(*canvas, is_preview);
  }

  if (params.display_header_footer) {
    // |page_number| is 0-based, so 1 is added.
    PrintHeaderAndFooter(canvas.get(), page_number + 1,
        print_preview_context_.total_page_count(), scale_factor,
        page_layout_in_points, *header_footer_info_, params);
  }

  float webkit_scale_factor = RenderPageContent(frame, page_number, canvas_area,
                                                content_area, scale_factor,
                                                canvas.get());

  if (*actual_shrink <= 0 || webkit_scale_factor <= 0) {
    NOTREACHED() << "Printing page " << page_number << " failed.";
  } else {
    // While rendering certain plugins (PDF) to metafile, we might need to
    // set custom scale factor. Update |actual_shrink| with custom scale
    // if it is set on canvas.
    // TODO(gene): We should revisit this solution for the next versions.
    // Consider creating metafile of the right size (or resizable)
    // https://code.google.com/p/chromium/issues/detail?id=126037
    if (!MetafileSkiaWrapper::GetCustomScaleOnCanvas(
            *canvas, actual_shrink)) {
      // Update the dpi adjustment with the "page |actual_shrink|" calculated in
      // webkit.
      *actual_shrink /= (webkit_scale_factor * css_scale_factor);
    }
  }

  bool result = metafile->FinishPage();
  DCHECK(result);
}

bool PrintWebViewHelper::CopyMetafileDataToSharedMem(
    Metafile* metafile, base::SharedMemoryHandle* shared_mem_handle) {
  uint32 buf_size = metafile->GetDataSize();
  base::SharedMemory shared_buf;
  if (buf_size >= kMetafileMaxSize) {
    NOTREACHED() << "Buffer too large: " << buf_size;
    return false;
  }

  // Allocate a shared memory buffer to hold the generated metafile data.
  if (!shared_buf.CreateAndMapAnonymous(buf_size)) {
    NOTREACHED() << "Buffer allocation failed";
    return false;
  }

  // Copy the bits into shared memory.
  if (!metafile->GetData(shared_buf.memory(), buf_size)) {
    NOTREACHED() << "GetData() failed";
    shared_buf.Unmap();
    return false;
  }
  shared_buf.GiveToProcess(base::GetCurrentProcessHandle(), shared_mem_handle);
  shared_buf.Unmap();

  Send(new PrintHostMsg_DuplicateSection(routing_id(), *shared_mem_handle,
                                         shared_mem_handle));
  return true;
}

}  // namespace printing
