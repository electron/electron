// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_HANDLER_H_
#define ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_HANDLER_H_

#include <string>

#include "atom/browser/web_contents_zoom_controller.h"
#include "base/macros.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class ListValue;
}

namespace content {
struct StreamInfo;
}

namespace atom {

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
  void OnZoomLevelChanged(content::WebContents* web_contents, double level,
      bool is_temporary);
  void AddObserver();
  void RemoveObserver();
  std::unique_ptr<base::Value> initialize_callback_id_;
  content::StreamInfo* stream_;
  std::string original_url_;

  DISALLOW_COPY_AND_ASSIGN(PdfViewerHandler);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_HANDLER_H_
