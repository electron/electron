// Copyright (c) 2015 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_pdf_document_helper_client.h"

#include "chrome/browser/pdf/pdf_frame_util.h"
#include "content/public/browser/web_contents.h"

ElectronPDFDocumentHelperClient::ElectronPDFDocumentHelperClient() = default;
ElectronPDFDocumentHelperClient::~ElectronPDFDocumentHelperClient() = default;

content::RenderFrameHost* ElectronPDFDocumentHelperClient::FindPdfFrame(
    content::WebContents* contents) {
  content::RenderFrameHost* main_frame = contents->GetPrimaryMainFrame();
  content::RenderFrameHost* pdf_frame =
      pdf_frame_util::FindPdfChildFrame(main_frame);
  return pdf_frame ? pdf_frame : main_frame;
}

void ElectronPDFDocumentHelperClient::UpdateContentRestrictions(
    content::RenderFrameHost* render_frame_host,
    int content_restrictions) {}
void ElectronPDFDocumentHelperClient::OnPDFHasUnsupportedFeature(
    content::WebContents* contents) {}
void ElectronPDFDocumentHelperClient::OnSaveURL(
    content::WebContents* contents) {}
void ElectronPDFDocumentHelperClient::SetPluginCanSave(
    content::RenderFrameHost* render_frame_host,
    bool can_save) {}
