// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_PRINT_PREVIEW_MESSAGE_HANDLER_H_
#define ATOM_BROWSER_ATOM_PRINT_PREVIEW_MESSAGE_HANDLER_H_

#include <map>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/printing/print_preview_message_handler.h"
#include "content/public/browser/web_contents_user_data.h"
#include "v8/include/v8.h"

namespace atom {

// Manages the print preview handling for a WebContents.
class AtomPrintPreviewMessageHandler
    : public printing::PrintPreviewMessageHandler,
      public content::WebContentsUserData<AtomPrintPreviewMessageHandler> {
 public:
  ~AtomPrintPreviewMessageHandler() override;

  using PrintToPDFCallback =
      base::Callback<void(v8::Local<v8::Value>, v8::Local<v8::Value>)>;

  void PrintToPDF(const base::DictionaryValue& options,
                  const PrintToPDFCallback& callback);

 private:
  explicit AtomPrintPreviewMessageHandler(content::WebContents* web_contents);
  friend class content::WebContentsUserData<AtomPrintPreviewMessageHandler>;
  typedef std::map<int, PrintToPDFCallback> PrintToPDFCallbackMap;

  void OnMetafileReadyForPrinting(
      content::RenderFrameHost* render_frame_host,
      const PrintHostMsg_DidPreviewDocument_Params& params,
      const PrintHostMsg_PreviewIds& ids) override;
  void OnPrintPreviewFailed(int document_cookie,
                            const PrintHostMsg_PreviewIds& ids) override;
  void RunPrintToPDFCallback(int request_id, uint32_t data_size, char* data);

  PrintToPDFCallbackMap print_to_pdf_callback_map_;

  base::WeakPtrFactory<PrintPreviewMessageHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AtomPrintPreviewMessageHandler);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_PRINT_PREVIEW_MESSAGE_HANDLER_H_
