// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_UI_H_
#define ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_UI_H_

#ifndef ENABLE_PDF_VIEWER
#error("This header can only be used when enable_pdf_viewer gyp flag is enabled")  // NOLINT
#endif  // defined(ENABLE_PDF_VIEWER)

#include <string>

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui_controller.h"
#include "ipc/ipc_message.h"

namespace content {
class BrowserContext;
struct StreamInfo;
}  // namespace content

namespace atom {

class PdfViewerHandler;

class PdfViewerUI : public content::WebUIController,
                    public content::WebContentsObserver {
 public:
  PdfViewerUI(content::BrowserContext* browser_context,
              content::WebUI* web_ui,
              const std::string& src);
  ~PdfViewerUI() override;

  // content::WebContentsObserver:
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;
  void RenderFrameCreated(content::RenderFrameHost* rfh) override;

 private:
  using StreamResponseCallback =
      base::OnceCallback<void(std::unique_ptr<content::StreamInfo>)>;
  class ResourceRequester;

  void OnPdfStreamCreated(std::unique_ptr<content::StreamInfo> stream_info);
  void OnSaveURLAs(const GURL& url, const content::Referrer& referrer);

  // Source URL from where the PDF originates.
  std::string src_;

  PdfViewerHandler* pdf_handler_;

  scoped_refptr<ResourceRequester> resource_requester_;

  // Pdf Resource stream.
  std::unique_ptr<content::StreamInfo> stream_;

  DISALLOW_COPY_AND_ASSIGN(PdfViewerUI);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_UI_H_
