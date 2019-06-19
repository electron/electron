// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_WEBUI_PDF_VIEWER_HANDLER_H_
#define SHELL_BROWSER_UI_WEBUI_PDF_VIEWER_HANDLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "shell/browser/web_contents_zoom_controller.h"

namespace base {
class ListValue;
}

namespace content {
struct StreamInfo;
}

namespace electron {

class PdfViewerHandler : public content::WebUIMessageHandler,
                         public WebContentsZoomController::Observer {
 public:
  explicit PdfViewerHandler(const std::string& original_url);
  ~PdfViewerHandler() override;

  void SetPdfResourceStream(content::StreamInfo* stream);

 protected:
  // WebUIMessageHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

 private:
  void Initialize(const base::ListValue* args);
  void GetDefaultZoom(const base::ListValue* args);
  void GetInitialZoom(const base::ListValue* args);
  void SetZoom(const base::ListValue* args);
  void GetStrings(const base::ListValue* args);
  void Reload(const base::ListValue* args);
  void OnZoomLevelChanged(content::WebContents* web_contents,
                          double level,
                          bool is_temporary);
  void AddObserver();
  void RemoveObserver();
  std::unique_ptr<base::Value> initialize_callback_id_;
  content::StreamInfo* stream_ = nullptr;
  std::string original_url_;

  DISALLOW_COPY_AND_ASSIGN(PdfViewerHandler);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_WEBUI_PDF_VIEWER_HANDLER_H_
