// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/printing_handler.h"

#include <stdint.h>
#include <utility>

#include "base/files/file_util.h"
#include "build/build_config.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_utility_printing_messages.h"
#include "chrome/utility/cloud_print/bitmap_image.h"
#include "chrome/utility/cloud_print/pwg_encoder.h"
#include "content/public/utility/utility_thread.h"
#include "pdf/pdf.h"
#include "printing/page_range.h"
#include "printing/pdf_render_settings.h"

#if defined(OS_WIN)
#include "printing/emf_win.h"
#include "ui/gfx/gdi_util.h"
#endif

#if defined(ENABLE_PRINT_PREVIEW)
#include "chrome/common/crash_keys.h"
#include "printing/backend/print_backend.h"
#endif

namespace printing {

namespace {

bool Send(IPC::Message* message) {
  return content::UtilityThread::Get()->Send(message);
}

void ReleaseProcessIfNeeded() {
  content::UtilityThread::Get()->ReleaseProcessIfNeeded();
}

}  // namespace

PrintingHandler::PrintingHandler() {}

PrintingHandler::~PrintingHandler() {}

bool PrintingHandler::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintingHandler, message)
#if defined(OS_WIN)
    IPC_MESSAGE_HANDLER(ChromeUtilityMsg_RenderPDFPagesToMetafiles,
                        OnRenderPDFPagesToMetafile)
    IPC_MESSAGE_HANDLER(ChromeUtilityMsg_RenderPDFPagesToMetafiles_GetPage,
                        OnRenderPDFPagesToMetafileGetPage)
    IPC_MESSAGE_HANDLER(ChromeUtilityMsg_RenderPDFPagesToMetafiles_Stop,
                        OnRenderPDFPagesToMetafileStop)
#endif  // OS_WIN
#if defined(ENABLE_PRINT_PREVIEW)
    IPC_MESSAGE_HANDLER(ChromeUtilityMsg_RenderPDFPagesToPWGRaster,
                        OnRenderPDFPagesToPWGRaster)
    IPC_MESSAGE_HANDLER(ChromeUtilityMsg_GetPrinterCapsAndDefaults,
                        OnGetPrinterCapsAndDefaults)
    IPC_MESSAGE_HANDLER(ChromeUtilityMsg_GetPrinterSemanticCapsAndDefaults,
                        OnGetPrinterSemanticCapsAndDefaults)
#endif  // ENABLE_PRINT_PREVIEW
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

#if defined(OS_WIN)
void PrintingHandler::OnRenderPDFPagesToMetafile(
    IPC::PlatformFileForTransit pdf_transit,
    const PdfRenderSettings& settings) {
  pdf_rendering_settings_ = settings;
  base::File pdf_file = IPC::PlatformFileForTransitToFile(pdf_transit);
  int page_count = LoadPDF(pdf_file.Pass());
  Send(
      new ChromeUtilityHostMsg_RenderPDFPagesToMetafiles_PageCount(page_count));
}

void PrintingHandler::OnRenderPDFPagesToMetafileGetPage(
    int page_number,
    IPC::PlatformFileForTransit output_file) {
  base::File emf_file = IPC::PlatformFileForTransitToFile(output_file);
  float scale_factor = 1.0f;
  bool success =
      RenderPdfPageToMetafile(page_number, emf_file.Pass(), &scale_factor);
  Send(new ChromeUtilityHostMsg_RenderPDFPagesToMetafiles_PageDone(
      success, scale_factor));
}

void PrintingHandler::OnRenderPDFPagesToMetafileStop() {
  ReleaseProcessIfNeeded();
}

#endif  // OS_WIN

#if defined(ENABLE_PRINT_PREVIEW)
void PrintingHandler::OnRenderPDFPagesToPWGRaster(
    IPC::PlatformFileForTransit pdf_transit,
    const PdfRenderSettings& settings,
    const PwgRasterSettings& bitmap_settings,
    IPC::PlatformFileForTransit bitmap_transit) {
  base::File pdf = IPC::PlatformFileForTransitToFile(pdf_transit);
  base::File bitmap = IPC::PlatformFileForTransitToFile(bitmap_transit);
  if (RenderPDFPagesToPWGRaster(std::move(pdf), settings, bitmap_settings,
                                std::move(bitmap))) {
    Send(new ChromeUtilityHostMsg_RenderPDFPagesToPWGRaster_Succeeded());
  } else {
    Send(new ChromeUtilityHostMsg_RenderPDFPagesToPWGRaster_Failed());
  }
  ReleaseProcessIfNeeded();
}
#endif  // ENABLE_PRINT_PREVIEW

#if defined(OS_WIN)
int PrintingHandler::LoadPDF(base::File pdf_file) {
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

bool PrintingHandler::RenderPdfPageToMetafile(int page_number,
                                              base::File output_file,
                                              float* scale_factor) {
  Emf metafile;
  metafile.Init();

  // We need to scale down DC to fit an entire page into DC available area.
  // Current metafile is based on screen DC and have current screen size.
  // Writing outside of those boundaries will result in the cut-off output.
  // On metafiles (this is the case here), scaling down will still record
  // original coordinates and we'll be able to print in full resolution.
  // Before playback we'll need to counter the scaling up that will happen
  // in the service (print_system_win.cc).
  *scale_factor =
      gfx::CalculatePageScale(metafile.context(),
                              pdf_rendering_settings_.area().right(),
                              pdf_rendering_settings_.area().bottom());
  gfx::ScaleDC(metafile.context(), *scale_factor);

  // The underlying metafile is of type Emf and ignores the arguments passed
  // to StartPage.
  metafile.StartPage(gfx::Size(), gfx::Rect(), 1);
  if (!chrome_pdf::RenderPDFPageToDC(
          &pdf_data_.front(),
          pdf_data_.size(),
          page_number,
          metafile.context(),
          pdf_rendering_settings_.dpi(),
          pdf_rendering_settings_.area().x(),
          pdf_rendering_settings_.area().y(),
          pdf_rendering_settings_.area().width(),
          pdf_rendering_settings_.area().height(),
          true,
          false,
          true,
          true,
          pdf_rendering_settings_.autorotate())) {
    return false;
  }
  metafile.FinishPage();
  metafile.FinishDocument();
  return metafile.SaveTo(&output_file);
}

#endif  // OS_WIN

#if defined(ENABLE_PRINT_PREVIEW)
bool PrintingHandler::RenderPDFPagesToPWGRaster(
    base::File pdf_file,
    const PdfRenderSettings& settings,
    const PwgRasterSettings& bitmap_settings,
    base::File bitmap_file) {
  bool autoupdate = true;
  base::File::Info info;
  if (!pdf_file.GetInfo(&info) || info.size <= 0 ||
      info.size > std::numeric_limits<int>::max())
    return false;
  int data_size = static_cast<int>(info.size);

  std::string data(data_size, 0);
  if (pdf_file.Read(0, &data[0], data_size) != data_size)
    return false;

  int total_page_count = 0;
  if (!chrome_pdf::GetPDFDocInfo(data.data(), data_size, &total_page_count,
                                 nullptr)) {
    return false;
  }

  cloud_print::PwgEncoder encoder;
  std::string pwg_header;
  encoder.EncodeDocumentHeader(&pwg_header);
  int bytes_written = bitmap_file.WriteAtCurrentPos(pwg_header.data(),
                                                    pwg_header.size());
  if (bytes_written != static_cast<int>(pwg_header.size()))
    return false;

  cloud_print::BitmapImage image(settings.area().size(),
                                 cloud_print::BitmapImage::BGRA);
  for (int i = 0; i < total_page_count; ++i) {
    int page_number = i;

    if (bitmap_settings.reverse_page_order) {
      page_number = total_page_count - 1 - page_number;
    }

    if (!chrome_pdf::RenderPDFPageToBitmap(data.data(),
                                           data_size,
                                           page_number,
                                           image.pixel_data(),
                                           image.size().width(),
                                           image.size().height(),
                                           settings.dpi(),
                                           autoupdate)) {
      return false;
    }

    cloud_print::PwgHeaderInfo header_info;
    header_info.dpi = settings.dpi();
    header_info.total_pages = total_page_count;

    // Transform odd pages.
    if (page_number % 2) {
      switch (bitmap_settings.odd_page_transform) {
        case TRANSFORM_NORMAL:
          break;
        case TRANSFORM_ROTATE_180:
          header_info.flipx = true;
          header_info.flipy = true;
          break;
        case TRANSFORM_FLIP_HORIZONTAL:
          header_info.flipx = true;
          break;
        case TRANSFORM_FLIP_VERTICAL:
          header_info.flipy = true;
          break;
      }
    }

    if (bitmap_settings.rotate_all_pages) {
      header_info.flipx = !header_info.flipx;
      header_info.flipy = !header_info.flipy;
    }

    std::string pwg_page;
    if (!encoder.EncodePage(image, header_info, &pwg_page))
      return false;
    bytes_written = bitmap_file.WriteAtCurrentPos(pwg_page.data(),
                                                  pwg_page.size());
    if (bytes_written != static_cast<int>(pwg_page.size()))
      return false;
  }
  return true;
}

void PrintingHandler::OnGetPrinterCapsAndDefaults(
    const std::string& printer_name) {
  scoped_refptr<PrintBackend> print_backend =
      PrintBackend::CreateInstance(nullptr);
  PrinterCapsAndDefaults printer_info;

  crash_keys::ScopedPrinterInfo crash_key(
      print_backend->GetPrinterDriverInfo(printer_name));

  if (print_backend->GetPrinterCapsAndDefaults(printer_name, &printer_info)) {
    Send(new ChromeUtilityHostMsg_GetPrinterCapsAndDefaults_Succeeded(
        printer_name, printer_info));
  } else  {
    Send(new ChromeUtilityHostMsg_GetPrinterCapsAndDefaults_Failed(
        printer_name));
  }
  ReleaseProcessIfNeeded();
}

void PrintingHandler::OnGetPrinterSemanticCapsAndDefaults(
    const std::string& printer_name) {
  scoped_refptr<PrintBackend> print_backend =
      PrintBackend::CreateInstance(nullptr);
  PrinterSemanticCapsAndDefaults printer_info;

  crash_keys::ScopedPrinterInfo crash_key(
      print_backend->GetPrinterDriverInfo(printer_name));

  if (print_backend->GetPrinterSemanticCapsAndDefaults(printer_name,
                                                       &printer_info)) {
    Send(new ChromeUtilityHostMsg_GetPrinterSemanticCapsAndDefaults_Succeeded(
        printer_name, printer_info));
  } else {
    Send(new ChromeUtilityHostMsg_GetPrinterSemanticCapsAndDefaults_Failed(
        printer_name));
  }
  ReleaseProcessIfNeeded();
}
#endif  // ENABLE_PRINT_PREVIEW

}  // namespace printing
