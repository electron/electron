// Copyright (c) 2015 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_PDF_DOCUMENT_HELPER_CLIENT_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_PDF_DOCUMENT_HELPER_CLIENT_H_

#include "components/pdf/browser/pdf_document_helper_client.h"
#include "services/screen_ai/buildflags/buildflags.h"

namespace content {
class WebContents;
}

class ElectronPDFDocumentHelperClient : public pdf::PDFDocumentHelperClient {
 public:
  ElectronPDFDocumentHelperClient();
  ~ElectronPDFDocumentHelperClient() override;

 private:
  // pdf::PDFDocumentHelperClient
  void UpdateContentRestrictions(content::RenderFrameHost* render_frame_host,
                                 int content_restrictions) override;
  void OnSaveURL() override {}
  void SetPluginCanSave(content::RenderFrameHost* render_frame_host,
                        bool can_save) override;
#if BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)
  void OnSearchifyStarted(content::RenderFrameHost* render_frame_host) override;
#endif
};

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_PDF_DOCUMENT_HELPER_CLIENT_H_
