// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_
#define CHROME_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_

#include <map>

#include "atom/browser/api/atom_api_web_contents.h"
#include "base/compiler_specific.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

struct PrintHostMsg_DidPreviewDocument_Params;

namespace content {
class WebContents;
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

  void PrintToPDF(const base::DictionaryValue& options,
                  const atom::api::WebContents::PrintToPDFCallback& callback);

 private:
  typedef std::map<int, atom::api::WebContents::PrintToPDFCallback>
      PrintToPDFCallbackMap;

  explicit PrintPreviewMessageHandler(content::WebContents* web_contents);
  friend class content::WebContentsUserData<PrintPreviewMessageHandler>;

  // Message handlers.
  void OnMetafileReadyForPrinting(
      const PrintHostMsg_DidPreviewDocument_Params& params);
  void OnPrintPreviewFailed(int document_cookie, int request_id);

  void RunPrintToPDFCallback(int request_id, uint32_t data_size, char* data);

  PrintToPDFCallbackMap print_to_pdf_callback_map_;

  DISALLOW_COPY_AND_ASSIGN(PrintPreviewMessageHandler);
};

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_
