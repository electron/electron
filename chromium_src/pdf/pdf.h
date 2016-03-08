// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_PDF_H_
#define PDF_PDF_H_

#include "ppapi/c/ppb.h"
#include "ppapi/cpp/module.h"

namespace chrome_pdf {

class PDFModule : public pp::Module {
 public:
  PDFModule();
  ~PDFModule() override;

  // pp::Module implementation.
  bool Init() override;
  pp::Instance* CreateInstance(PP_Instance instance) override;
};

int PPP_InitializeModule(PP_Module module_id,
                         PPB_GetInterface get_browser_interface);
void PPP_ShutdownModule();
const void* PPP_GetInterface(const char* interface_name);

#if defined(OS_WIN)
// |pdf_buffer| is the buffer that contains the entire PDF document to be
//     rendered.
// |buffer_size| is the size of |pdf_buffer| in bytes.
// |page_number| is the 0-based index of the page to be rendered.
// |dc| is the device context to render into.
// |dpi| and |dpi_y| is the resolution. If the value is -1, the dpi from the DC
//     will be used.
// |bounds_origin_x|, |bounds_origin_y|, |bounds_width| and |bounds_height|
//     specify a bounds rectangle within the DC in which to render the PDF
//     page.
// |fit_to_bounds| specifies whether the output should be shrunk to fit the
//     supplied bounds if the page size is larger than the bounds in any
//     dimension. If this is false, parts of the PDF page that lie outside
//     the bounds will be clipped.
// |stretch_to_bounds| specifies whether the output should be stretched to fit
//     the supplied bounds if the page size is smaller than the bounds in any
//     dimension.
// If both |fit_to_bounds| and |stretch_to_bounds| are true, then
//     |fit_to_bounds| is honored first.
// |keep_aspect_ratio| If any scaling is to be done is true, this flag
//     specifies whether the original aspect ratio of the page should be
//     preserved while scaling.
// |center_in_bounds| specifies whether the final image (after any scaling is
//     done) should be centered within the given bounds.
// |autorotate| specifies whether the final image should be rotated to match
//     the output bound.
// Returns false if the document or the page number are not valid.
bool RenderPDFPageToDC(const void* pdf_buffer,
                       int buffer_size,
                       int page_number,
                       HDC dc,
                       int dpi,
                       int bounds_origin_x,
                       int bounds_origin_y,
                       int bounds_width,
                       int bounds_height,
                       bool fit_to_bounds,
                       bool stretch_to_bounds,
                       bool keep_aspect_ratio,
                       bool center_in_bounds,
                       bool autorotate);
#endif
// |page_count| and |max_page_width| are optional and can be NULL.
// Returns false if the document is not valid.
bool GetPDFDocInfo(const void* pdf_buffer,
                   int buffer_size, int* page_count,
                   double* max_page_width);

// Gets the dimensions of a specific page in a document.
// |pdf_buffer| is the buffer that contains the entire PDF document to be
//     rendered.
// |pdf_buffer_size| is the size of |pdf_buffer| in bytes.
// |page_number| is the page number that the function will get the dimensions
//     of.
// |width| is the output for the width of the page in points.
// |height| is the output for the height of the page in points.
// Returns false if the document or the page number are not valid.
bool GetPDFPageSizeByIndex(const void* pdf_buffer,
                           int pdf_buffer_size, int page_number,
                           double* width, double* height);

// Renders PDF page into 4-byte per pixel BGRA color bitmap.
// |pdf_buffer| is the buffer that contains the entire PDF document to be
//     rendered.
// |pdf_buffer_size| is the size of |pdf_buffer| in bytes.
// |page_number| is the 0-based index of the page to be rendered.
// |bitmap_buffer| is the output buffer for bitmap.
// |bitmap_width| is the width of the output bitmap.
// |bitmap_height| is the height of the output bitmap.
// |dpi| is the resolutions.
// |autorotate| specifies whether the final image should be rotated to match
//     the output bound.
// Returns false if the document or the page number are not valid.
bool RenderPDFPageToBitmap(const void* pdf_buffer,
                           int pdf_buffer_size,
                           int page_number,
                           void* bitmap_buffer,
                           int bitmap_width,
                           int bitmap_height,
                           int dpi,
                           bool autorotate);

}  // namespace chrome_pdf

#endif  // PDF_PDF_H_
