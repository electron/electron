// Copyright (c) 2015 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_pdf_web_contents_helper_client.h"

ElectronPDFWebContentsHelperClient::ElectronPDFWebContentsHelperClient() =
    default;
ElectronPDFWebContentsHelperClient::~ElectronPDFWebContentsHelperClient() =
    default;

content::RenderFrameHost* ElectronPDFWebContentsHelperClient::FindPdfFrame(
    content::WebContents* contents) {
  return nullptr;
}

void ElectronPDFWebContentsHelperClient::UpdateContentRestrictions(
    content::WebContents* contents,
    int content_restrictions) {}
void ElectronPDFWebContentsHelperClient::OnPDFHasUnsupportedFeature(
    content::WebContents* contents) {}
void ElectronPDFWebContentsHelperClient::OnSaveURL(
    content::WebContents* contents) {}
void ElectronPDFWebContentsHelperClient::SetPluginCanSave(
    content::WebContents* contents,
    bool can_save) {}
