// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_PRINTING_HANDLER_WIN_H_
#define CHROME_UTILITY_PRINTING_HANDLER_WIN_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/utility/utility_message_handler.h"
#include "ipc/ipc_platform_file.h"
#include "printing/pdf_render_settings.h"

namespace printing {
class PdfRenderSettings;
struct PwgRasterSettings;
struct PageRange;
}

// Dispatches IPCs for printing.
class PrintingHandlerWin : public UtilityMessageHandler {
 public:
  PrintingHandlerWin();
  ~PrintingHandlerWin() override;

  // IPC::Listener:
  bool OnMessageReceived(const IPC::Message& message) override;

  static void PrintingHandlerWin::PreSandboxStartup();

 private:
  // IPC message handlers.
  void OnRenderPDFPagesToMetafile(IPC::PlatformFileForTransit pdf_transit,
                                  const printing::PdfRenderSettings& settings);
  void OnRenderPDFPagesToMetafileGetPage(
      int page_number,
      IPC::PlatformFileForTransit output_file);
  void OnRenderPDFPagesToMetafileStop();

  int LoadPDF(base::File pdf_file);
  bool RenderPdfPageToMetafile(int page_number,
                               base::File output_file,
                               float* scale_factor);

  std::vector<char> pdf_data_;
  printing::PdfRenderSettings pdf_rendering_settings_;

  DISALLOW_COPY_AND_ASSIGN(PrintingHandlerWin);
};

#endif  // CHROME_UTILITY_PRINTING_HANDLER_WIN_H_
