// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/webui/pdf_viewer_handler.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/values.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/page_zoom.h"

namespace atom {

PdfViewerHandler::PdfViewerHandler(const std::string& stream_url,
                                   const std::string& original_url)
    : stream_url_(stream_url), original_url_(original_url) {}

PdfViewerHandler::~PdfViewerHandler() {}

void PdfViewerHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "initialize",
      base::Bind(&PdfViewerHandler::Initialize, base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getDefaultZoom",
      base::Bind(&PdfViewerHandler::GetInitialZoom, base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getInitialZoom",
      base::Bind(&PdfViewerHandler::GetInitialZoom, base::Unretained(this)));
}

void PdfViewerHandler::OnJavascriptAllowed() {
  auto host_zoom_map =
      content::HostZoomMap::GetForWebContents(web_ui()->GetWebContents());
  host_zoom_map_subscription_ =
      host_zoom_map->AddZoomLevelChangedCallback(base::Bind(
          &PdfViewerHandler::OnZoomLevelChanged, base::Unretained(this)));
}

void PdfViewerHandler::OnJavascriptDisallowed() {
  host_zoom_map_subscription_.reset();
}

void PdfViewerHandler::Initialize(const base::ListValue* args) {
  AllowJavascript();

  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));
  std::unique_ptr<base::DictionaryValue> stream_info(new base::DictionaryValue);
  stream_info->SetString("streamURL", stream_url_);
  stream_info->SetString("originalURL", original_url_);
  ResolveJavascriptCallback(*callback_id, *stream_info);
}

void PdfViewerHandler::GetDefaultZoom(const base::ListValue* args) {
  if (!IsJavascriptAllowed())
    return;
  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  auto host_zoom_map =
      content::HostZoomMap::GetForWebContents(web_ui()->GetWebContents());
  double zoom_level = host_zoom_map->GetDefaultZoomLevel();
  ResolveJavascriptCallback(
      *callback_id,
      base::FundamentalValue(content::ZoomLevelToZoomFactor(zoom_level)));
}

void PdfViewerHandler::GetInitialZoom(const base::ListValue* args) {
  if (!IsJavascriptAllowed())
    return;
  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  double zoom_level =
      content::HostZoomMap::GetZoomLevel(web_ui()->GetWebContents());
  ResolveJavascriptCallback(
      *callback_id,
      base::FundamentalValue(content::ZoomLevelToZoomFactor(zoom_level)));
}

void PdfViewerHandler::OnZoomLevelChanged(
    const content::HostZoomMap::ZoomLevelChange& change) {
  // TODO(deepak1556): This will work only if zoom level is changed through host
  // zoom map.
  if (change.scheme == "chrome" && change.host == "pdf-viewer") {
    CallJavascriptFunction(
        "cr.webUIListenerCallback", base::StringValue("onZoomLevelChanged"),
        base::FundamentalValue(
            content::ZoomLevelToZoomFactor(change.zoom_level)));
  }
}

}  // namespace atom
