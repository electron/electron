// Copyright (c) 2015 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_pdf_document_helper_client.h"

#include "content/public/browser/web_contents.h"
#include "pdf/content_restriction.h"
#include "shell/browser/api/electron_api_web_contents.h"

ElectronPDFDocumentHelperClient::ElectronPDFDocumentHelperClient() = default;
ElectronPDFDocumentHelperClient::~ElectronPDFDocumentHelperClient() = default;

void ElectronPDFDocumentHelperClient::UpdateContentRestrictions(
    content::RenderFrameHost* render_frame_host,
    int content_restrictions) {
  // UpdateContentRestrictions potentially gets called twice from
  // pdf/pdf_view_web_plugin.cc.  The first time it is potentially called is
  // when loading starts and it is called with a restriction on printing.  The
  // second time it is called is when loading is finished and if printing is
  // allowed there won't be a printing restriction passed, so we can use this
  // second call to notify that the pdf document is ready to print.
  if (!(content_restrictions & chrome_pdf::kContentRestrictionPrint)) {
    content::WebContents* web_contents =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    electron::api::WebContents* api_web_contents =
        electron::api::WebContents::From(
            web_contents->GetOutermostWebContents());
    if (api_web_contents) {
      api_web_contents->PDFReadyToPrint();
    }
  }
}
