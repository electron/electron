// Copyright (c) 2015 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_pdf_document_helper_client.h"

#include "chrome/browser/pdf/pdf_viewer_stream_manager.h"
#include "chrome/common/content_restriction.h"
#include "components/pdf/browser/pdf_frame_util.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "pdf/content_restriction.h"
#include "pdf/pdf_features.h"
#include "shell/browser/api/electron_api_web_contents.h"

ElectronPDFDocumentHelperClient::ElectronPDFDocumentHelperClient() = default;
ElectronPDFDocumentHelperClient::~ElectronPDFDocumentHelperClient() = default;

void ElectronPDFDocumentHelperClient::UpdateContentRestrictions(
    content::RenderFrameHost* render_frame_host,
    int content_restrictions) {
  // UpdateContentRestrictions potentially gets called twice from
  // pdf/pdf_view_web_plugin.cc. The first time it is potentially called is
  // when loading starts and it is called with a restriction on printing. The
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

void ElectronPDFDocumentHelperClient::SetPluginCanSave(
    content::RenderFrameHost* render_frame_host,
    bool can_save) {
  if (chrome_pdf::features::IsOopifPdfEnabled()) {
    auto* pdf_viewer_stream_manager =
        pdf::PdfViewerStreamManager::FromWebContents(
            content::WebContents::FromRenderFrameHost(render_frame_host));
    if (!pdf_viewer_stream_manager) {
      return;
    }

    content::RenderFrameHost* embedder_host =
        pdf_frame_util::GetEmbedderHost(render_frame_host);
    CHECK(embedder_host);

    pdf_viewer_stream_manager->SetPluginCanSave(embedder_host, can_save);
    return;
  }

  auto* guest_view =
      extensions::MimeHandlerViewGuest::FromRenderFrameHost(render_frame_host);
  if (guest_view) {
    guest_view->SetPluginCanSave(can_save);
  }
}
