// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_HANDLER_H_
#define ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class ListValue;
}

namespace atom {

class PdfViewerHandler : public content::WebUIMessageHandler {
 public:
  PdfViewerHandler(const std::string& stream_url,
                   const std::string& original_url);
  ~PdfViewerHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

 private:
  void Initialize(const base::ListValue* args);
  void GetDefaultZoom(const base::ListValue* args);
  void GetInitialZoom(const base::ListValue* args);
  void OnZoomLevelChanged(const content::HostZoomMap::ZoomLevelChange& change);

  // Keeps track of events related to zooming.
  std::unique_ptr<content::HostZoomMap::Subscription>
      host_zoom_map_subscription_;
  std::string stream_url_;
  std::string original_url_;

  DISALLOW_COPY_AND_ASSIGN(PdfViewerHandler);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WEBUI_PDF_VIEWER_HANDLER_H_
