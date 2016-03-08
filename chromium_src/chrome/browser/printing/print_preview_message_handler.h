// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_
#define CHROME_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class PrintPreviewUI;
struct PrintHostMsg_DidGetPreviewPageCount_Params;
struct PrintHostMsg_DidPreviewDocument_Params;
struct PrintHostMsg_DidPreviewPage_Params;
struct PrintHostMsg_RequestPrintPreview_Params;
struct PrintHostMsg_SetOptionsFromDocument_Params;

namespace content {
class WebContents;
}

namespace gfx {
class Rect;
}

namespace printing {

struct PageSizeMargins;

// Manages the print preview handling for a WebContents.
class PrintPreviewMessageHandler
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PrintPreviewMessageHandler> {
 public:
  ~PrintPreviewMessageHandler() override;

  // content::WebContentsObserver implementation.
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  explicit PrintPreviewMessageHandler(content::WebContents* web_contents);
  friend class content::WebContentsUserData<PrintPreviewMessageHandler>;

  // Gets the print preview dialog associated with the WebContents being
  // observed.
  content::WebContents* GetPrintPreviewDialog();

  // Gets the PrintPreviewUI associated with the WebContents being observed.
  PrintPreviewUI* GetPrintPreviewUI();

  // Message handlers.
  void OnRequestPrintPreview(
      const PrintHostMsg_RequestPrintPreview_Params& params);
  void OnDidGetDefaultPageLayout(const PageSizeMargins& page_layout_in_points,
                                 const gfx::Rect& printable_area_in_points,
                                 bool has_custom_page_size_style);
  void OnDidGetPreviewPageCount(
      const PrintHostMsg_DidGetPreviewPageCount_Params& params);
  void OnDidPreviewPage(const PrintHostMsg_DidPreviewPage_Params& params);
  void OnMetafileReadyForPrinting(
      const PrintHostMsg_DidPreviewDocument_Params& params);
  void OnPrintPreviewFailed(int document_cookie);
  void OnPrintPreviewCancelled(int document_cookie);
  void OnInvalidPrinterSettings(int document_cookie);
  void OnSetOptionsFromDocument(
      const PrintHostMsg_SetOptionsFromDocument_Params& params);

  DISALLOW_COPY_AND_ASSIGN(PrintPreviewMessageHandler);
};

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_
