// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_
#define ATOM_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_

#include <map>

#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "components/services/pdf_compositor/public/interfaces/pdf_compositor.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "v8/include/v8.h"

struct PrintHostMsg_DidPreviewDocument_Params;
struct PrintHostMsg_PreviewIds;

namespace content {
class RenderFrameHost;
}

namespace atom {

// Manages the print preview handling for a WebContents.
class PrintPreviewMessageHandler
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PrintPreviewMessageHandler> {
 public:
  using PrintToPDFCallback =
      base::Callback<void(v8::Local<v8::Value>, v8::Local<v8::Value>)>;

  ~PrintPreviewMessageHandler() override;

  void PrintToPDF(const base::DictionaryValue& options,
                  const PrintToPDFCallback& callback);

 protected:
  // content::WebContentsObserver implementation.
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

 private:
  friend class content::WebContentsUserData<PrintPreviewMessageHandler>;

  explicit PrintPreviewMessageHandler(content::WebContents* web_contents);

  void OnMetafileReadyForPrinting(
      content::RenderFrameHost* render_frame_host,
      const PrintHostMsg_DidPreviewDocument_Params& params,
      const PrintHostMsg_PreviewIds& ids);
  void OnCompositePdfDocumentDone(const PrintHostMsg_PreviewIds& ids,
                                  printing::mojom::PdfCompositor::Status status,
                                  base::ReadOnlySharedMemoryRegion region);
  void OnPrintPreviewFailed(int document_cookie,
                            const PrintHostMsg_PreviewIds& ids);
  void OnPrintPreviewCancelled(int document_cookie,
                               const PrintHostMsg_PreviewIds& ids);
  void RunPrintToPDFCallback(int request_id,
                             scoped_refptr<base::RefCountedMemory> data_bytes);

  using PrintToPDFCallbackMap = std::map<int, PrintToPDFCallback>;
  PrintToPDFCallbackMap print_to_pdf_callback_map_;

  base::WeakPtrFactory<PrintPreviewMessageHandler> weak_ptr_factory_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(PrintPreviewMessageHandler);
};

}  // namespace atom

#endif  // ATOM_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_
