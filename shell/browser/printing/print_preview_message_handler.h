// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_
#define ELECTRON_SHELL_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_

#include <map>

#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "components/printing/common/print.mojom.h"
#include "components/services/print_compositor/public/mojom/print_compositor.mojom.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "printing/mojom/print.mojom.h"
#include "shell/common/gin_helper/promise.h"
#include "v8/include/v8.h"

namespace content {
class RenderFrameHost;
}

namespace electron {

// Manages the print preview handling for a WebContents.
class PrintPreviewMessageHandler
    : public printing::mojom::PrintPreviewUI,
      public content::WebContentsUserData<PrintPreviewMessageHandler> {
 public:
  ~PrintPreviewMessageHandler() override;

  // disable copy
  PrintPreviewMessageHandler(const PrintPreviewMessageHandler&) = delete;
  PrintPreviewMessageHandler& operator=(const PrintPreviewMessageHandler&) =
      delete;

  void PrintToPDF(base::DictionaryValue options,
                  gin_helper::Promise<v8::Local<v8::Value>> promise);

 private:
  friend class content::WebContentsUserData<PrintPreviewMessageHandler>;

  explicit PrintPreviewMessageHandler(content::WebContents* web_contents);

  void OnCompositeDocumentToPdfDone(
      int32_t request_id,
      printing::mojom::PrintCompositor::Status status,
      base::ReadOnlySharedMemoryRegion region);
  void OnPrepareForDocumentToPdfDone(
      int32_t request_id,
      printing::mojom::PrintCompositor::Status status);
  void OnCompositePdfPageDone(int page_number,
                              int document_cookie,
                              int32_t request_id,
                              printing::mojom::PrintCompositor::Status status,
                              base::ReadOnlySharedMemoryRegion region);

  // printing::mojo::PrintPreviewUI:
  void SetOptionsFromDocument(
      const printing::mojom::OptionsFromDocumentParamsPtr params,
      int32_t request_id) override {}
  void PrintPreviewFailed(int32_t document_cookie, int32_t request_id) override;
  void PrintPreviewCancelled(int32_t document_cookie,
                             int32_t request_id) override;
  void PrinterSettingsInvalid(int32_t document_cookie,
                              int32_t request_id) override {}
  void DidPrepareDocumentForPreview(int32_t document_cookie,
                                    int32_t request_id) override;
  void DidPreviewPage(printing::mojom::DidPreviewPageParamsPtr params,
                      int32_t request_id) override;
  void MetafileReadyForPrinting(
      printing::mojom::DidPreviewDocumentParamsPtr params,
      int32_t request_id) override;
  void DidGetDefaultPageLayout(
      printing::mojom::PageSizeMarginsPtr page_layout_in_points,
      const gfx::Rect& printable_area_in_points,
      bool has_custom_page_size_style,
      int32_t request_id) override {}
  void DidStartPreview(printing::mojom::DidStartPreviewParamsPtr params,
                       int32_t request_id) override {}

  gin_helper::Promise<v8::Local<v8::Value>> GetPromise(int request_id);

  void ResolvePromise(int request_id,
                      scoped_refptr<base::RefCountedMemory> data_bytes);
  void RejectPromise(int request_id);

  using PromiseMap = std::map<int, gin_helper::Promise<v8::Local<v8::Value>>>;
  PromiseMap promise_map_;

  // TODO(clavin): refactor to use the WebContents provided by the
  // WebContentsUserData base class instead of storing a duplicate ref
  content::WebContents* web_contents_ = nullptr;

  mojo::AssociatedRemote<printing::mojom::PrintRenderFrame> print_render_frame_;

  mojo::AssociatedReceiver<printing::mojom::PrintPreviewUI> receiver_{this};

  base::WeakPtrFactory<PrintPreviewMessageHandler> weak_ptr_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_PRINTING_PRINT_PREVIEW_MESSAGE_HANDLER_H_
