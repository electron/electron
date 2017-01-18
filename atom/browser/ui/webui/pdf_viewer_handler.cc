// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/webui/pdf_viewer_handler.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/values.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"

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
      "getTabId",
      base::Bind(&PdfViewerHandler::GetTabId, base::Unretained(this)));
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

void PdfViewerHandler::GetTabId(const base::ListValue* args) {
  if (!IsJavascriptAllowed())
    return;
  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));
  ResolveJavascriptCallback(*callback_id, base::FundamentalValue(-1));
}

}  // namespace atom
