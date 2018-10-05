// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included message file, so no include guard.

#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "build/build_config.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_param_traits.h"
#include "ipc/ipc_platform_file.h"
#include "printing/backend/print_backend.h"
#include "printing/buildflags/buildflags.h"
#include "printing/page_range.h"
#include "printing/pdf_render_settings.h"
#include "printing/pwg_raster_settings.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#define IPC_MESSAGE_START ChromeUtilityPrintingMsgStart

IPC_ENUM_TRAITS_MAX_VALUE(printing::PdfRenderSettings::Mode,
                          printing::PdfRenderSettings::Mode::LAST)

IPC_STRUCT_TRAITS_BEGIN(printing::PdfRenderSettings)
  IPC_STRUCT_TRAITS_MEMBER(area)
  IPC_STRUCT_TRAITS_MEMBER(offsets)
  IPC_STRUCT_TRAITS_MEMBER(dpi)
  IPC_STRUCT_TRAITS_MEMBER(autorotate)
  IPC_STRUCT_TRAITS_MEMBER(mode)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(printing::PrinterCapsAndDefaults)
  IPC_STRUCT_TRAITS_MEMBER(printer_capabilities)
  IPC_STRUCT_TRAITS_MEMBER(caps_mime_type)
  IPC_STRUCT_TRAITS_MEMBER(printer_defaults)
  IPC_STRUCT_TRAITS_MEMBER(defaults_mime_type)
IPC_STRUCT_TRAITS_END()

IPC_ENUM_TRAITS_MAX_VALUE(printing::ColorModel, printing::PROCESSCOLORMODEL_RGB)

IPC_STRUCT_TRAITS_BEGIN(printing::PrinterSemanticCapsAndDefaults::Paper)
  IPC_STRUCT_TRAITS_MEMBER(display_name)
  IPC_STRUCT_TRAITS_MEMBER(vendor_id)
  IPC_STRUCT_TRAITS_MEMBER(size_um)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(printing::PrinterSemanticCapsAndDefaults)
  IPC_STRUCT_TRAITS_MEMBER(collate_capable)
  IPC_STRUCT_TRAITS_MEMBER(collate_default)
  IPC_STRUCT_TRAITS_MEMBER(copies_capable)
  IPC_STRUCT_TRAITS_MEMBER(duplex_capable)
  IPC_STRUCT_TRAITS_MEMBER(duplex_default)
  IPC_STRUCT_TRAITS_MEMBER(color_changeable)
  IPC_STRUCT_TRAITS_MEMBER(color_default)
  IPC_STRUCT_TRAITS_MEMBER(color_model)
  IPC_STRUCT_TRAITS_MEMBER(bw_model)
  IPC_STRUCT_TRAITS_MEMBER(papers)
  IPC_STRUCT_TRAITS_MEMBER(default_paper)
  IPC_STRUCT_TRAITS_MEMBER(dpis)
  IPC_STRUCT_TRAITS_MEMBER(default_dpi)
IPC_STRUCT_TRAITS_END()

IPC_ENUM_TRAITS_MAX_VALUE(printing::PwgRasterTransformType,
                          printing::TRANSFORM_TYPE_LAST)

IPC_STRUCT_TRAITS_BEGIN(printing::PwgRasterSettings)
  IPC_STRUCT_TRAITS_MEMBER(odd_page_transform)
  IPC_STRUCT_TRAITS_MEMBER(rotate_all_pages)
  IPC_STRUCT_TRAITS_MEMBER(reverse_page_order)
IPC_STRUCT_TRAITS_END()

#if defined(OS_WIN)
// Reply when the utility process loaded PDF. |page_count| is 0, if loading
// failed.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_RenderPDFPagesToMetafiles_PageCount,
                     int /* page_count */)

// Reply when the utility process rendered the PDF page.
IPC_MESSAGE_CONTROL2(ChromeUtilityHostMsg_RenderPDFPagesToMetafiles_PageDone,
                     bool /* success */,
                     float /* scale_factor */)

// Request that the given font characters be loaded by the browser so it's
// cached by the OS. Please see
// PdfToEmfUtilityProcessHostClient::OnPreCacheFontCharacters for details.
#if 0
IPC_SYNC_MESSAGE_CONTROL2_0(ChromeUtilityHostMsg_PreCacheFontCharacters,
                            LOGFONT /* font_data */,
                            base::string16 /* characters */)
#endif

// Tell the utility process to start rendering the given PDF into a metafile.
// Utility process would be alive until
// ChromeUtilityMsg_RenderPDFPagesToMetafiles_Stop message.
IPC_MESSAGE_CONTROL2(ChromeUtilityMsg_RenderPDFPagesToMetafiles,
                     IPC::PlatformFileForTransit /* input_file */,
                     printing::PdfRenderSettings /* settings */)

// Requests conversion of the next page.
IPC_MESSAGE_CONTROL2(ChromeUtilityMsg_RenderPDFPagesToMetafiles_GetPage,
                     int /* page_number */,
                     IPC::PlatformFileForTransit /* output_file */)

// Requests utility process to stop conversion and exit.
IPC_MESSAGE_CONTROL0(ChromeUtilityMsg_RenderPDFPagesToMetafiles_Stop)

#endif  // OS_WIN
