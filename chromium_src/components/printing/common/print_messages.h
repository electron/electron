// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// IPC messages for printing.
// Multiply-included message file, hence no include guard.

#include <stdint.h>

#include <string>
#include <vector>

#include "base/memory/shared_memory.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/printing/common/printing_param_traits_macros.h"
#include "ipc/ipc_message_macros.h"
#include "printing/page_range.h"
#include "printing/page_size_margins.h"
#include "printing/print_job_constants.h"
#include "third_party/WebKit/public/web/WebPrintScalingOption.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/ipc/gfx_param_traits.h"
#include "ui/gfx/native_widget_types.h"

// Force multiple inclusion of the param traits file to generate all methods.
#undef COMPONENTS_PRINTING_COMMON_PRINTING_PARAM_TRAITS_MACROS_H_

#ifndef COMPONENTS_PRINTING_COMMON_PRINT_MESSAGES_H_
#define COMPONENTS_PRINTING_COMMON_PRINT_MESSAGES_H_

struct PrintMsg_Print_Params {
  PrintMsg_Print_Params();
  ~PrintMsg_Print_Params();

  // Resets the members of the struct to 0.
  void Reset();

  gfx::Size page_size;
  gfx::Size content_size;
  gfx::Rect printable_area;
  int margin_top;
  int margin_left;
  double dpi;
  double min_shrink;
  double max_shrink;
  int desired_dpi;
  int document_cookie;
  bool selection_only;
  bool supports_alpha_blend;
  int32_t preview_ui_id;
  int preview_request_id;
  bool is_first_request;
  blink::WebPrintScalingOption print_scaling_option;
  bool print_to_pdf;
  bool display_header_footer;
  base::string16 title;
  base::string16 url;
  bool should_print_backgrounds;
};

struct PrintMsg_PrintPages_Params {
  PrintMsg_PrintPages_Params();
  ~PrintMsg_PrintPages_Params();

  // Resets the members of the struct to 0.
  void Reset();

  PrintMsg_Print_Params params;
  std::vector<int> pages;
};

#if defined(ENABLE_PRINT_PREVIEW)
struct PrintHostMsg_RequestPrintPreview_Params {
  PrintHostMsg_RequestPrintPreview_Params();
  ~PrintHostMsg_RequestPrintPreview_Params();
  bool is_modifiable;
  bool webnode_only;
  bool has_selection;
  bool selection_only;
};

struct PrintHostMsg_SetOptionsFromDocument_Params {
  PrintHostMsg_SetOptionsFromDocument_Params();
  ~PrintHostMsg_SetOptionsFromDocument_Params();

  bool is_scaling_disabled;
  int copies;
  printing::DuplexMode duplex;
  printing::PageRanges page_ranges;
};
#endif  // defined(ENABLE_PRINT_PREVIEW)

#endif  // COMPONENTS_PRINTING_COMMON_PRINT_MESSAGES_H_

#define IPC_MESSAGE_START PrintMsgStart

IPC_ENUM_TRAITS_MAX_VALUE(blink::WebPrintScalingOption,
                          blink::WebPrintScalingOptionLast)

// Parameters for a render request.
IPC_STRUCT_TRAITS_BEGIN(PrintMsg_Print_Params)
  // Physical size of the page, including non-printable margins,
  // in pixels according to dpi.
  IPC_STRUCT_TRAITS_MEMBER(page_size)

  // In pixels according to dpi_x and dpi_y.
  IPC_STRUCT_TRAITS_MEMBER(content_size)

  // Physical printable area of the page in pixels according to dpi.
  IPC_STRUCT_TRAITS_MEMBER(printable_area)

  // The y-offset of the printable area, in pixels according to dpi.
  IPC_STRUCT_TRAITS_MEMBER(margin_top)

  // The x-offset of the printable area, in pixels according to dpi.
  IPC_STRUCT_TRAITS_MEMBER(margin_left)

  // Specifies dots per inch.
  IPC_STRUCT_TRAITS_MEMBER(dpi)

  // Minimum shrink factor. See PrintSettings::min_shrink for more information.
  IPC_STRUCT_TRAITS_MEMBER(min_shrink)

  // Maximum shrink factor. See PrintSettings::max_shrink for more information.
  IPC_STRUCT_TRAITS_MEMBER(max_shrink)

  // Desired apparent dpi on paper.
  IPC_STRUCT_TRAITS_MEMBER(desired_dpi)

  // Cookie for the document to ensure correctness.
  IPC_STRUCT_TRAITS_MEMBER(document_cookie)

  // Should only print currently selected text.
  IPC_STRUCT_TRAITS_MEMBER(selection_only)

  // Does the printer support alpha blending?
  IPC_STRUCT_TRAITS_MEMBER(supports_alpha_blend)

  // *** Parameters below are used only for print preview. ***

  // The print preview ui associated with this request.
  IPC_STRUCT_TRAITS_MEMBER(preview_ui_id)

  // The id of the preview request.
  IPC_STRUCT_TRAITS_MEMBER(preview_request_id)

  // True if this is the first preview request.
  IPC_STRUCT_TRAITS_MEMBER(is_first_request)

  // Specifies the page scaling option for preview printing.
  IPC_STRUCT_TRAITS_MEMBER(print_scaling_option)

  // True if print to pdf is requested.
  IPC_STRUCT_TRAITS_MEMBER(print_to_pdf)

  // Specifies if the header and footer should be rendered.
  IPC_STRUCT_TRAITS_MEMBER(display_header_footer)

  // Title string to be printed as header if requested by the user.
  IPC_STRUCT_TRAITS_MEMBER(title)

  // URL string to be printed as footer if requested by the user.
  IPC_STRUCT_TRAITS_MEMBER(url)

  // True if print backgrounds is requested by the user.
  IPC_STRUCT_TRAITS_MEMBER(should_print_backgrounds)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_BEGIN(PrintMsg_PrintPage_Params)
  // Parameters to render the page as a printed page. It must always be the same
  // value for all the document.
  IPC_STRUCT_MEMBER(PrintMsg_Print_Params, params)

  // The page number is the indicator of the square that should be rendered
  // according to the layout specified in PrintMsg_Print_Params.
  IPC_STRUCT_MEMBER(int, page_number)
IPC_STRUCT_END()

IPC_STRUCT_TRAITS_BEGIN(printing::PageRange)
  IPC_STRUCT_TRAITS_MEMBER(from)
  IPC_STRUCT_TRAITS_MEMBER(to)
IPC_STRUCT_TRAITS_END()

#if defined(ENABLE_PRINT_PREVIEW)
IPC_STRUCT_TRAITS_BEGIN(PrintHostMsg_RequestPrintPreview_Params)
  IPC_STRUCT_TRAITS_MEMBER(is_modifiable)
  IPC_STRUCT_TRAITS_MEMBER(webnode_only)
  IPC_STRUCT_TRAITS_MEMBER(has_selection)
  IPC_STRUCT_TRAITS_MEMBER(selection_only)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(PrintHostMsg_SetOptionsFromDocument_Params)
  // Specifies whether print scaling is enabled or not.
  IPC_STRUCT_TRAITS_MEMBER(is_scaling_disabled)

  // Specifies number of copies to be printed.
  IPC_STRUCT_TRAITS_MEMBER(copies)

  // Specifies paper handling option.
  IPC_STRUCT_TRAITS_MEMBER(duplex)

  // Specifies page range to be printed.
  IPC_STRUCT_TRAITS_MEMBER(page_ranges)
IPC_STRUCT_TRAITS_END()
#endif  // defined(ENABLE_PRINT_PREVIEW)

IPC_STRUCT_TRAITS_BEGIN(printing::PageSizeMargins)
  IPC_STRUCT_TRAITS_MEMBER(content_width)
  IPC_STRUCT_TRAITS_MEMBER(content_height)
  IPC_STRUCT_TRAITS_MEMBER(margin_left)
  IPC_STRUCT_TRAITS_MEMBER(margin_right)
  IPC_STRUCT_TRAITS_MEMBER(margin_top)
  IPC_STRUCT_TRAITS_MEMBER(margin_bottom)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(PrintMsg_PrintPages_Params)
  // Parameters to render the page as a printed page. It must always be the same
  // value for all the document.
  IPC_STRUCT_TRAITS_MEMBER(params)

  // If empty, this means a request to render all the printed pages.
  IPC_STRUCT_TRAITS_MEMBER(pages)
IPC_STRUCT_TRAITS_END()

#if defined(ENABLE_PRINT_PREVIEW)
// Parameters to describe a rendered document.
IPC_STRUCT_BEGIN(PrintHostMsg_DidPreviewDocument_Params)
  // A shared memory handle to metafile data.
  IPC_STRUCT_MEMBER(base::SharedMemoryHandle, metafile_data_handle)

  // Size of metafile data.
  IPC_STRUCT_MEMBER(uint32_t, data_size)

  // Cookie for the document to ensure correctness.
  IPC_STRUCT_MEMBER(int, document_cookie)

  // Store the expected pages count.
  IPC_STRUCT_MEMBER(int, expected_pages_count)

  // Whether the preview can be modified.
  IPC_STRUCT_MEMBER(bool, modifiable)

  // The id of the preview request.
  IPC_STRUCT_MEMBER(int, preview_request_id)
IPC_STRUCT_END()

// Parameters to describe a rendered preview page.
IPC_STRUCT_BEGIN(PrintHostMsg_DidPreviewPage_Params)
  // A shared memory handle to metafile data for a draft document of the page.
  IPC_STRUCT_MEMBER(base::SharedMemoryHandle, metafile_data_handle)

  // Size of metafile data.
  IPC_STRUCT_MEMBER(uint32_t, data_size)

  // |page_number| is zero-based and should not be negative.
  IPC_STRUCT_MEMBER(int, page_number)

  // The id of the preview request.
  IPC_STRUCT_MEMBER(int, preview_request_id)
IPC_STRUCT_END()

// Parameters sent along with the page count.
IPC_STRUCT_BEGIN(PrintHostMsg_DidGetPreviewPageCount_Params)
  // Cookie for the document to ensure correctness.
  IPC_STRUCT_MEMBER(int, document_cookie)

  // Total page count.
  IPC_STRUCT_MEMBER(int, page_count)

  // Indicates whether the previewed document is modifiable.
  IPC_STRUCT_MEMBER(bool, is_modifiable)

  // The id of the preview request.
  IPC_STRUCT_MEMBER(int, preview_request_id)

  // Indicates whether the existing preview data needs to be cleared or not.
  IPC_STRUCT_MEMBER(bool, clear_preview_data)
IPC_STRUCT_END()
#endif  // defined(ENABLE_PRINT_PREVIEW)

// Parameters to describe a rendered page.
IPC_STRUCT_BEGIN(PrintHostMsg_DidPrintPage_Params)
  // A shared memory handle to the EMF data. This data can be quite large so a
  // memory map needs to be used.
  IPC_STRUCT_MEMBER(base::SharedMemoryHandle, metafile_data_handle)

  // Size of the metafile data.
  IPC_STRUCT_MEMBER(uint32_t, data_size)

  // Cookie for the document to ensure correctness.
  IPC_STRUCT_MEMBER(int, document_cookie)

  // Page number.
  IPC_STRUCT_MEMBER(int, page_number)

  // The size of the page the page author specified.
  IPC_STRUCT_MEMBER(gfx::Size, page_size)

  // The printable area the page author specified.
  IPC_STRUCT_MEMBER(gfx::Rect, content_area)
IPC_STRUCT_END()

// TODO(dgn) Rename *ScriptedPrint messages because they are not called only
//           from scripts.
// Parameters for the IPC message ViewHostMsg_ScriptedPrint
IPC_STRUCT_BEGIN(PrintHostMsg_ScriptedPrint_Params)
  IPC_STRUCT_MEMBER(int, cookie)
  IPC_STRUCT_MEMBER(int, expected_pages_count)
  IPC_STRUCT_MEMBER(bool, has_selection)
  IPC_STRUCT_MEMBER(bool, is_scripted)
  IPC_STRUCT_MEMBER(printing::MarginType, margin_type)
IPC_STRUCT_END()


// Messages sent from the browser to the renderer.

#if defined(ENABLE_PRINT_PREVIEW)
// Tells the render view to initiate print preview for the entire document.
IPC_MESSAGE_ROUTED1(PrintMsg_InitiatePrintPreview, bool /* selection_only */)
#endif  // defined(ENABLE_PRINT_PREVIEW)

// Tells the render frame to initiate printing or print preview for a particular
// node, depending on which mode the render frame is in.
IPC_MESSAGE_ROUTED0(PrintMsg_PrintNodeUnderContextMenu)

#if defined(ENABLE_BASIC_PRINTING) && defined(ENABLE_PRINT_PREVIEW)
// Tells the renderer to print the print preview tab's PDF plugin without
// showing the print dialog. (This is the final step in the print preview
// workflow.)
IPC_MESSAGE_ROUTED1(PrintMsg_PrintForPrintPreview,
                    base::DictionaryValue /* settings */)
#endif  // defined(ENABLE_BASIC_PRINTING) && defined(ENABLE_PRINT_PREVIEW)

#if defined(ENABLE_BASIC_PRINTING)
// Tells the render view to switch the CSS to print media type, renders every
// requested pages and switch back the CSS to display media type.
IPC_MESSAGE_ROUTED0(PrintMsg_PrintPages)

// Like PrintMsg_PrintPages, but using the print preview document's frame/node.
IPC_MESSAGE_ROUTED0(PrintMsg_PrintForSystemDialog)
#endif  // defined(ENABLE_BASIC_PRINTING)

// Tells the render view that printing is done so it can clean up.
IPC_MESSAGE_ROUTED1(PrintMsg_PrintingDone,
                    bool /* success */)

// Tells the render view whether scripted printing is blocked or not.
IPC_MESSAGE_ROUTED1(PrintMsg_SetScriptedPrintingBlocked,
                    bool /* blocked */)

#if defined(ENABLE_PRINT_PREVIEW)
// Tells the render view to switch the CSS to print media type, renders every
// requested pages for print preview using the given |settings|. This gets
// called multiple times as the user updates settings.
IPC_MESSAGE_ROUTED1(PrintMsg_PrintPreview,
                    base::DictionaryValue /* settings */)
#endif  // defined(ENABLE_PRINT_PREVIEW)

// Messages sent from the renderer to the browser.

#if defined(OS_WIN)
// Duplicates a shared memory handle from the renderer to the browser. Then
// the renderer can flush the handle.
IPC_SYNC_MESSAGE_ROUTED1_1(PrintHostMsg_DuplicateSection,
                           base::SharedMemoryHandle /* renderer handle */,
                           base::SharedMemoryHandle /* browser handle */)
#endif

// Check if printing is enabled.
IPC_SYNC_MESSAGE_ROUTED0_1(PrintHostMsg_IsPrintingEnabled,
                           bool /* is_enabled */)

// Tells the browser that the renderer is done calculating the number of
// rendered pages according to the specified settings.
IPC_MESSAGE_ROUTED2(PrintHostMsg_DidGetPrintedPagesCount,
                    int /* rendered document cookie */,
                    int /* number of rendered pages */)

// Sends the document cookie of the current printer query to the browser.
IPC_MESSAGE_ROUTED1(PrintHostMsg_DidGetDocumentCookie,
                    int /* rendered document cookie */)

// Tells the browser that the print dialog has been shown.
IPC_MESSAGE_ROUTED0(PrintHostMsg_DidShowPrintDialog)

// Sends back to the browser the rendered "printed page" that was requested by
// a PrintMsg_PrintPages message or from scripted printing. The memory handle in
// this message is already valid in the browser process.
IPC_MESSAGE_ROUTED1(PrintHostMsg_DidPrintPage,
                    PrintHostMsg_DidPrintPage_Params /* page content */)

// The renderer wants to know the default print settings.
IPC_SYNC_MESSAGE_ROUTED0_1(PrintHostMsg_GetDefaultPrintSettings,
                           PrintMsg_Print_Params /* default_settings */)

// The renderer wants to update the current print settings with new
// |job_settings|.
IPC_SYNC_MESSAGE_ROUTED2_2(PrintHostMsg_UpdatePrintSettings,
                           int /* document_cookie */,
                           base::DictionaryValue /* job_settings */,
                           PrintMsg_PrintPages_Params /* current_settings */,
                           bool /* canceled */)

// It's the renderer that controls the printing process when it is generated
// by javascript. This step is about showing UI to the user to select the
// final print settings. The output parameter is the same as
// PrintMsg_PrintPages which is executed implicitly.
IPC_SYNC_MESSAGE_ROUTED1_1(PrintHostMsg_ScriptedPrint,
                           PrintHostMsg_ScriptedPrint_Params,
                           PrintMsg_PrintPages_Params
                               /* settings chosen by the user*/)

#if defined(OS_ANDROID)
// Asks the browser to create a temporary file for the renderer to fill
// in resulting PdfMetafileSkia in printing.
IPC_SYNC_MESSAGE_CONTROL1_2(PrintHostMsg_AllocateTempFileForPrinting,
                            int /* render_view_id */,
                            base::FileDescriptor /* temp file fd */,
                            int /* fd in browser*/)  // Used only by Chrome OS.
IPC_MESSAGE_CONTROL2(PrintHostMsg_TempFileForPrintingWritten,
                     int /* render_view_id */,
                     int /* fd in browser */)  // Used only by Chrome OS.
#endif  // defined(OS_ANDROID)

#if defined(ENABLE_PRINT_PREVIEW)
// Asks the browser to do print preview.
IPC_MESSAGE_ROUTED1(PrintHostMsg_RequestPrintPreview,
                    PrintHostMsg_RequestPrintPreview_Params /* params */)

// Notify the browser the number of pages in the print preview document.
IPC_MESSAGE_ROUTED1(PrintHostMsg_DidGetPreviewPageCount,
                    PrintHostMsg_DidGetPreviewPageCount_Params /* params */)

// Notify the browser of the default page layout according to the currently
// selected printer and page size.
// |printable_area_in_points| Specifies the printable area in points.
// |has_custom_page_size_style| is true when the printing frame has a custom
// page size css otherwise false.
IPC_MESSAGE_ROUTED3(PrintHostMsg_DidGetDefaultPageLayout,
                    printing::PageSizeMargins /* page layout in points */,
                    gfx::Rect /* printable area in points */,
                    bool /* has custom page size style */)

// Notify the browser a print preview page has been rendered.
IPC_MESSAGE_ROUTED1(PrintHostMsg_DidPreviewPage,
                    PrintHostMsg_DidPreviewPage_Params /* params */)

// Asks the browser whether the print preview has been cancelled.
IPC_SYNC_MESSAGE_ROUTED2_1(PrintHostMsg_CheckForCancel,
                           int32_t /* PrintPreviewUI ID */,
                           int /* request id */,
                           bool /* print preview cancelled */)

// Sends back to the browser the complete rendered document (non-draft mode,
// used for printing) that was requested by a PrintMsg_PrintPreview message.
// The memory handle in this message is already valid in the browser process.
IPC_MESSAGE_ROUTED1(PrintHostMsg_MetafileReadyForPrinting,
                    PrintHostMsg_DidPreviewDocument_Params /* params */)
#endif  // defined(ENABLE_PRINT_PREVIEW)

// This is sent when there are invalid printer settings.
IPC_MESSAGE_ROUTED0(PrintHostMsg_ShowInvalidPrinterSettingsError)

// Tell the browser printing failed.
IPC_MESSAGE_ROUTED1(PrintHostMsg_PrintingFailed,
                    int /* document cookie */)

#if defined(ENABLE_PRINT_PREVIEW)
// Tell the browser print preview failed.
IPC_MESSAGE_ROUTED1(PrintHostMsg_PrintPreviewFailed,
                    int /* document cookie */)

// Tell the browser print preview was cancelled.
IPC_MESSAGE_ROUTED1(PrintHostMsg_PrintPreviewCancelled,
                    int /* document cookie */)

// Tell the browser print preview found the selected printer has invalid
// settings (which typically caused by disconnected network printer or printer
// driver is bogus).
IPC_MESSAGE_ROUTED1(PrintHostMsg_PrintPreviewInvalidPrinterSettings,
                    int /* document cookie */)

// Run a nested message loop in the renderer until print preview for
// window.print() finishes.
IPC_SYNC_MESSAGE_ROUTED0_0(PrintHostMsg_SetupScriptedPrintPreview)

// Tell the browser to show the print preview, when the document is sufficiently
// loaded such that the renderer can determine whether it is modifiable or not.
IPC_MESSAGE_ROUTED1(PrintHostMsg_ShowScriptedPrintPreview,
                    bool /* is_modifiable */)

// Notify the browser to set print presets based on source PDF document.
IPC_MESSAGE_ROUTED1(PrintHostMsg_SetOptionsFromDocument,
                    PrintHostMsg_SetOptionsFromDocument_Params /* params */)
#endif  // defined(ENABLE_PRINT_PREVIEW)
