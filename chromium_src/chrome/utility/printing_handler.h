// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_PRINTING_HANDLER_H_
#define CHROME_UTILITY_PRINTING_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
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
}

// Dispatches IPCs for printing.
class PrintingHandler : public UtilityMessageHandler {
 public:
  PrintingHandler();
  ~PrintingHandler() override;

  // IPC::Listener:
  bool OnMessageReceived(const IPC::Message& message) override;

  static void PrintingHandler::PreSandboxStartup();

 private:
  // IPC message handlers.
#if defined(OS_WIN)
  void OnRenderPDFPagesToMetafile(IPC::PlatformFileForTransit pdf_transit,
                                  const printing::PdfRenderSettings& settings);
  void OnRenderPDFPagesToMetafileGetPage(
      int page_number,
      IPC::PlatformFileForTransit output_file);
  void OnRenderPDFPagesToMetafileStop();
#endif  // OS_WIN

#if defined(OS_WIN)
  int LoadPDF(base::File pdf_file);
  bool RenderPdfPageToMetafile(int page_number,
                               base::File output_file,
                               float* scale_factor);
#endif  // OS_WIN

#if defined(OS_WIN)
  std::vector<char> pdf_data_;
  printing::PdfRenderSettings pdf_rendering_settings_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PrintingHandler);
};

#endif  // CHROME_UTILITY_PRINTING_HANDLER_H_
