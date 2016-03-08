// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_PRINTING_HANDLER_H_
#define CHROME_UTILITY_PRINTING_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/utility/utility_message_handler.h"
#include "ipc/ipc_platform_file.h"
#include "printing/pdf_render_settings.h"

#if !defined(ENABLE_PRINT_PREVIEW) && !defined(OS_WIN)
#error "Windows or full printing must be enabled"
#endif

namespace printing {

class PdfRenderSettings;
struct PwgRasterSettings;
struct PageRange;

// Dispatches IPCs for printing.
class PrintingHandler : public UtilityMessageHandler {
 public:
  PrintingHandler();
  ~PrintingHandler() override;

  // IPC::Listener:
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  // IPC message handlers.
#if defined(OS_WIN)
  void OnRenderPDFPagesToMetafile(IPC::PlatformFileForTransit pdf_transit,
                                  const PdfRenderSettings& settings);
  void OnRenderPDFPagesToMetafileGetPage(
      int page_number,
      IPC::PlatformFileForTransit output_file);
  void OnRenderPDFPagesToMetafileStop();
#endif  // OS_WIN
#if defined(ENABLE_PRINT_PREVIEW)
  void OnRenderPDFPagesToPWGRaster(IPC::PlatformFileForTransit pdf_transit,
                                   const PdfRenderSettings& settings,
                                   const PwgRasterSettings& bitmap_settings,
                                   IPC::PlatformFileForTransit bitmap_transit);
#endif  // ENABLE_PRINT_PREVIEW

#if defined(OS_WIN)
  int LoadPDF(base::File pdf_file);
  bool RenderPdfPageToMetafile(int page_number,
                               base::File output_file,
                               float* scale_factor);
#endif  // OS_WIN
#if defined(ENABLE_PRINT_PREVIEW)
  bool RenderPDFPagesToPWGRaster(base::File pdf_file,
                                 const PdfRenderSettings& settings,
                                 const PwgRasterSettings& bitmap_settings,
                                 base::File bitmap_file);

  void OnGetPrinterCapsAndDefaults(const std::string& printer_name);
  void OnGetPrinterSemanticCapsAndDefaults(const std::string& printer_name);
#endif  // ENABLE_PRINT_PREVIEW

#if defined(OS_WIN)
  std::vector<char> pdf_data_;
  PdfRenderSettings pdf_rendering_settings_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PrintingHandler);
};

}  // namespace printing

#endif  // CHROME_UTILITY_PRINTING_HANDLER_H_
