// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_UI_H_
#define ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_UI_H_

#include <string>

#include "atom/browser/loader/layered_resource_handler.h"
#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui_controller.h"
#include "ipc/ipc_message.h"

namespace content {
class BrowserContext;
class ResourceContext;
struct StreamInfo;
}

namespace net {
class HttpResponseHeaders;
}

namespace atom {

class PdfViewerHandler;

class PdfViewerUI : public content::WebUIController,
                    public content::WebContentsObserver,
                    public LayeredResourceHandler::Delegate {
 public:
  PdfViewerUI(content::BrowserContext* browser_context,
              content::WebUI* web_ui,
              const std::string& src);
  ~PdfViewerUI() override;

  // content::WebContentsObserver:
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;
  void RenderFrameCreated(content::RenderFrameHost* rfh) override;

  // LayeredResourceHandler:
  void OnResponseStarted(content::ResourceResponse* response) override;

 private:
  void OnPdfStreamCreated(std::unique_ptr<content::StreamInfo> stream_info);
  void OnPdfStreamResponseStarted(
      scoped_refptr<net::HttpResponseHeaders> headers,
      const std::string& mime_type);
  void OnSaveURLAs(const GURL& url, const content::Referrer& referrer);

  // Source URL from where the PDF originates.
  std::string src_;

  PdfViewerHandler* pdf_handler_;

  // Pdf Resource stream.
  std::unique_ptr<content::StreamInfo> stream_;

  DISALLOW_COPY_AND_ASSIGN(PdfViewerUI);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_UI_H_
