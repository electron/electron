// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_UI_H_
#define ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_UI_H_

#include <string>

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui_controller.h"
#include "ipc/ipc_message.h"

namespace content {
class BrowserContext;
struct StreamInfo;
}

namespace atom {

class PdfViewerUI : public content::WebUIController,
                    public content::WebContentsObserver {
 public:
  static const char kOrigin[];
  static const char kHost[];
  static const char kId[];
  static const char kSrc[];

  PdfViewerUI(content::BrowserContext* browser_context,
              content::WebUI* web_ui,
              const std::string& view_id,
              const std::string& src);
  ~PdfViewerUI() override;

  // content::WebContentsObserver:
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

 private:
  void OnSaveURLAs(const GURL& url, const content::Referrer& referrer);

  std::string src_;
  std::unique_ptr<content::StreamInfo> stream_;

  DISALLOW_COPY_AND_ASSIGN(PdfViewerUI);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_UI_H_
