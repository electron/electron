// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/printing_handler_win.h"

#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/path_service.h"
#include "base/scoped_native_library.h"
#include "chrome/common/chrome_utility_printing_messages.h"
#include "chrome/common/print_messages.h"
#include "content/public/utility/utility_thread.h"
#include "pdf/pdf.h"
#include "printing/emf_win.h"
#include "printing/page_range.h"
#include "printing/pdf_render_settings.h"
#include "ui/gfx/gdi_util.h"

namespace printing {

namespace {

bool Send(IPC::Message* message) {
  return content::UtilityThread::Get()->Send(message);
}

void ReleaseProcessIfNeeded() {
  content::UtilityThread::Get()->ReleaseProcess();
}

void PreCacheFontCharacters(const LOGFONT* logfont,
                            const wchar_t* text,
                            size_t text_length) {
#if 0
  Send(new ChromeUtilityHostMsg_PreCacheFontCharacters(
      *logfont, base::string16(text, text_length)));
#endif
}

}  // namespace

PrintingHandlerWin::PrintingHandlerWin() {
  chrome_pdf::SetPDFEnsureTypefaceCharactersAccessible(PreCacheFontCharacters);
}

PrintingHandlerWin::~PrintingHandlerWin() {}

bool PrintingHandlerWin::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintingHandlerWin, message)
    IPC_MESSAGE_HANDLER(ChromeUtilityMsg_RenderPDFPagesToMetafiles,
                        OnRenderPDFPagesToMetafile)
    IPC_MESSAGE_HANDLER(ChromeUtilityMsg_RenderPDFPagesToMetafiles_GetPage,
                        OnRenderPDFPagesToMetafileGetPage)
    IPC_MESSAGE_HANDLER(ChromeUtilityMsg_RenderPDFPagesToMetafiles_Stop,
                        OnRenderPDFPagesToMetafileStop)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PrintingHandlerWin::OnRenderPDFPagesToMetafile(
    IPC::PlatformFileForTransit pdf_transit,
    const PdfRenderSettings& settings) {
  pdf_rendering_settings_ = settings;
  chrome_pdf::SetPDFUseGDIPrinting(pdf_rendering_settings_.mode ==
                                   PdfRenderSettings::Mode::GDI_TEXT);
  int postscript_level;
  switch (pdf_rendering_settings_.mode) {
    case PdfRenderSettings::Mode::POSTSCRIPT_LEVEL2:
      postscript_level = chrome_pdf::PrintingMode::kPostScript2;
      break;
    case PdfRenderSettings::Mode::POSTSCRIPT_LEVEL3:
      postscript_level = chrome_pdf::PrintingMode::kPostScript3;
      break;
    default:
      postscript_level =
          chrome_pdf::PrintingMode::kEmf;  // Not using postscript.
  }
  chrome_pdf::SetPDFUsePrintMode(postscript_level);

  base::File pdf_file = IPC::PlatformFileForTransitToFile(pdf_transit);
  int page_count = LoadPDF(std::move(pdf_file));
  Send(
      new ChromeUtilityHostMsg_RenderPDFPagesToMetafiles_PageCount(page_count));
}

void PrintingHandlerWin::OnRenderPDFPagesToMetafileGetPage(
    int page_number,
    IPC::PlatformFileForTransit output_file) {
  base::File emf_file = IPC::PlatformFileForTransitToFile(output_file);
  float scale_factor = 1.0f;
  bool postscript = pdf_rendering_settings_.mode ==
                        PdfRenderSettings::Mode::POSTSCRIPT_LEVEL2 ||
                    pdf_rendering_settings_.mode ==
                        PdfRenderSettings::Mode::POSTSCRIPT_LEVEL3;
  bool success = RenderPdfPageToMetafile(page_number, std::move(emf_file),
                                         &scale_factor, postscript);
  Send(new ChromeUtilityHostMsg_RenderPDFPagesToMetafiles_PageDone(
      success, scale_factor));
}

void PrintingHandlerWin::OnRenderPDFPagesToMetafileStop() {
  ReleaseProcessIfNeeded();
}

int PrintingHandlerWin::LoadPDF(base::File pdf_file) {
  int64_t length64 = pdf_file.GetLength();
  if (length64 <= 0 || length64 > std::numeric_limits<int>::max())
    return 0;
  int length = static_cast<int>(length64);

  pdf_data_.resize(length);
  if (length != pdf_file.Read(0, pdf_data_.data(), pdf_data_.size()))
    return 0;

  int total_page_count = 0;
  if (!chrome_pdf::GetPDFDocInfo(&pdf_data_.front(), pdf_data_.size(),
                                 &total_page_count, nullptr)) {
    return 0;
  }
  return total_page_count;
}

bool PrintingHandlerWin::RenderPdfPageToMetafile(int page_number,
                                                 base::File output_file,
                                                 float* scale_factor,
                                                 bool postscript) {
  Emf metafile;
  metafile.Init();

  // We need to scale down DC to fit an entire page into DC available area.
  // Current metafile is based on screen DC and have current screen size.
  // Writing outside of those boundaries will result in the cut-off output.
  // On metafiles (this is the case here), scaling down will still record
  // original coordinates and we'll be able to print in full resolution.
  // Before playback we'll need to counter the scaling up that will happen
  // in the service (print_system_win.cc).
  //
  // The postscript driver does not use the metafile size since it outputs
  // postscript rather than a metafile. Instead it uses the printable area
  // sent to RenderPDFPageToDC to determine the area to render. Therefore,
  // don't scale the DC to match the metafile, and send the printer physical
  // offsets to the driver.
  if (!postscript) {
    *scale_factor = gfx::CalculatePageScale(
        metafile.context(), pdf_rendering_settings_.area.right(),
        pdf_rendering_settings_.area.bottom());
    gfx::ScaleDC(metafile.context(), *scale_factor);
  }

  // The underlying metafile is of type Emf and ignores the arguments passed
  // to StartPage.
  metafile.StartPage(gfx::Size(), gfx::Rect(), 1);
  int offset_x = postscript ? pdf_rendering_settings_.offsets.x() : 0;
  int offset_y = postscript ? pdf_rendering_settings_.offsets.y() : 0;

  if (!chrome_pdf::RenderPDFPageToDC(
          &pdf_data_.front(), pdf_data_.size(), page_number, metafile.context(),
          pdf_rendering_settings_.dpi.width(),
          pdf_rendering_settings_.dpi.height(),
          pdf_rendering_settings_.area.x() - offset_x,
          pdf_rendering_settings_.area.y() - offset_y,
          pdf_rendering_settings_.area.width(),
          pdf_rendering_settings_.area.height(), true, false, true, true,
          pdf_rendering_settings_.use_color,
          pdf_rendering_settings_.autorotate)) {
    return false;
  }
  metafile.FinishPage();
  metafile.FinishDocument();
  return metafile.SaveTo(&output_file);
}

}  // namespace printing
