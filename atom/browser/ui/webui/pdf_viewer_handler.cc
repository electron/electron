// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/webui/pdf_viewer_handler.h"

#include <memory>
#include <utility>

#include "atom/common/atom_constants.h"
#include "base/bind.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/stream_handle.h"
#include "content/public/browser/stream_info.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/page_zoom.h"
#include "content/public/common/url_constants.h"
#include "net/http/http_response_headers.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"

namespace atom {

namespace {

void CreateResponseHeadersDictionary(const net::HttpResponseHeaders* headers,
                                     base::DictionaryValue* result) {
  if (!headers)
    return;

  size_t iter = 0;
  std::string header_name;
  std::string header_value;
  while (headers->EnumerateHeaderLines(&iter, &header_name, &header_value)) {
    base::Value* existing_value = nullptr;
    if (result->Get(header_name, &existing_value)) {
      std::string src = existing_value->GetString();
      result->SetString(header_name, src + ", " + header_value);
    } else {
      result->SetString(header_name, header_value);
    }
  }
}

void PopulateStreamInfo(base::DictionaryValue* stream_info,
                        content::StreamInfo* stream,
                        const std::string& original_url) {
  auto headers_dict = std::make_unique<base::DictionaryValue>();
  auto stream_url = stream->handle->GetURL().spec();
  CreateResponseHeadersDictionary(stream->response_headers.get(),
                                  headers_dict.get());
  stream_info->SetString("streamURL", stream_url);
  stream_info->SetString("originalURL", original_url);
  stream_info->Set("responseHeaders", std::move(headers_dict));
}

}  // namespace

PdfViewerHandler::PdfViewerHandler(const std::string& src)
    : original_url_(src) {}

PdfViewerHandler::~PdfViewerHandler() {
  RemoveObserver();
}

void PdfViewerHandler::SetPdfResourceStream(content::StreamInfo* stream) {
  stream_ = stream;
  if (!!initialize_callback_id_.get()) {
    auto list = std::make_unique<base::ListValue>();
    list->Set(0, std::move(initialize_callback_id_));
    Initialize(list.get());
  }
}

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
  web_ui()->RegisterMessageCallback(
      "setZoom",
      base::Bind(&PdfViewerHandler::SetZoom, base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getStrings",
      base::Bind(&PdfViewerHandler::GetStrings, base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "reload", base::Bind(&PdfViewerHandler::Reload, base::Unretained(this)));
}

void PdfViewerHandler::OnJavascriptAllowed() {
  AddObserver();
}

void PdfViewerHandler::OnJavascriptDisallowed() {
  RemoveObserver();
}

void PdfViewerHandler::Initialize(const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  if (stream_) {
    CHECK(!initialize_callback_id_.get());
    AllowJavascript();

    auto stream_info = std::make_unique<base::DictionaryValue>();
    PopulateStreamInfo(stream_info.get(), stream_, original_url_);
    ResolveJavascriptCallback(*callback_id, *stream_info);
  } else {
    initialize_callback_id_ =
        base::Value::ToUniquePtrValue(callback_id.Clone());
  }

  auto zoom_controller =
      WebContentsZoomController::FromWebContents(web_ui()->GetWebContents());
  zoom_controller->SetZoomMode(WebContentsZoomController::ZOOM_MODE_MANUAL);
  zoom_controller->SetZoomLevel(0);
}

void PdfViewerHandler::GetDefaultZoom(const base::ListValue* args) {
  if (!IsJavascriptAllowed())
    return;
  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  auto zoom_controller =
      WebContentsZoomController::FromWebContents(web_ui()->GetWebContents());
  double zoom_level = zoom_controller->GetDefaultZoomLevel();
  ResolveJavascriptCallback(
      *callback_id, base::Value(content::ZoomLevelToZoomFactor(zoom_level)));
}

void PdfViewerHandler::GetInitialZoom(const base::ListValue* args) {
  if (!IsJavascriptAllowed())
    return;
  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  auto zoom_controller =
      WebContentsZoomController::FromWebContents(web_ui()->GetWebContents());
  double zoom_level = zoom_controller->GetZoomLevel();
  ResolveJavascriptCallback(
      *callback_id, base::Value(content::ZoomLevelToZoomFactor(zoom_level)));
}

void PdfViewerHandler::SetZoom(const base::ListValue* args) {
  if (!IsJavascriptAllowed())
    return;
  CHECK_EQ(2U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));
  double zoom_level = 0.0;
  CHECK(args->GetDouble(1, &zoom_level));

  auto zoom_controller =
      WebContentsZoomController::FromWebContents(web_ui()->GetWebContents());
  zoom_controller->SetZoomLevel(zoom_level);
  ResolveJavascriptCallback(*callback_id, base::Value(zoom_level));
}

void PdfViewerHandler::GetStrings(const base::ListValue* args) {
  if (!IsJavascriptAllowed())
    return;
  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  auto result = std::make_unique<base::DictionaryValue>();
// TODO(deepak1556): Generate strings from components/pdf_strings.grdp.
#define SET_STRING(id, resource) result->SetString(id, resource)
  SET_STRING("passwordPrompt",
             "This document is password protected.  Please enter a password.");
  SET_STRING("passwordSubmit", "Submit");
  SET_STRING("passwordInvalid", "Incorrect password");
  SET_STRING("pageLoading", "Loading...");
  SET_STRING("pageLoadFailed", "Failed to load PDF document");
  SET_STRING("pageReload", "Reload");
  SET_STRING("bookmarks", "Bookmarks");
  SET_STRING("labelPageNumber", "Page number");
  SET_STRING("tooltipRotateCW", "Rotate clockwise");
  SET_STRING("tooltipDownload", "Download");
  SET_STRING("tooltipFitToPage", "Fit to page");
  SET_STRING("tooltipFitToWidth", "Fit to width");
  SET_STRING("tooltipZoomIn", "Zoom in");
  SET_STRING("tooltipZoomOut", "Zoom out");
#undef SET_STRING

  webui::SetLoadTimeDataDefaults(g_browser_process->GetApplicationLocale(),
                                 result.get());
  ResolveJavascriptCallback(*callback_id, *result);
}

void PdfViewerHandler::Reload(const base::ListValue* args) {
  CHECK_EQ(0U, args->GetSize());
  web_ui()->GetWebContents()->ReloadFocusedFrame(false);
}

void PdfViewerHandler::OnZoomLevelChanged(content::WebContents* web_contents,
                                          double level,
                                          bool is_temporary) {
  if (web_ui()->GetWebContents() == web_contents) {
    CallJavascriptFunction("cr.webUIListenerCallback",
                           base::Value("onZoomLevelChanged"),
                           base::Value(content::ZoomLevelToZoomFactor(level)));
  }
}

void PdfViewerHandler::AddObserver() {
  auto zoom_controller =
      WebContentsZoomController::FromWebContents(web_ui()->GetWebContents());
  zoom_controller->AddObserver(this);
}

void PdfViewerHandler::RemoveObserver() {
  auto zoom_controller =
      WebContentsZoomController::FromWebContents(web_ui()->GetWebContents());
  zoom_controller->RemoveObserver(this);
}

}  // namespace atom
