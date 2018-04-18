// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ENABLE_PDF_VIEWER
#error("This header can only be used when enable_pdf_viewer gyp flag is enabled")
#endif  // defined(ENABLE_PDF_VIEWER)

#ifndef COMPONENTS_PDF_RENDERER_PEPPER_PDF_HOST_H_
#define COMPONENTS_PDF_RENDERER_PEPPER_PDF_HOST_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "ppapi/host/resource_host.h"

namespace content {
class RenderFrame;
class RendererPpapiHost;
}  // namespace content

namespace pdf {

class PdfAccessibilityTree;

class PepperPDFHost : public ppapi::host::ResourceHost {
 public:
  PepperPDFHost(content::RendererPpapiHost* host,
                PP_Instance instance,
                PP_Resource resource);
  ~PepperPDFHost() override;

  // ppapi::host::ResourceHost:
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  int32_t OnHostMsgDidStartLoading(ppapi::host::HostMessageContext* context);
  int32_t OnHostMsgDidStopLoading(ppapi::host::HostMessageContext* context);
  int32_t OnHostMsgSaveAs(ppapi::host::HostMessageContext* context);
  int32_t OnHostMsgSetSelectedText(ppapi::host::HostMessageContext* context,
                                   const base::string16& selected_text);
  int32_t OnHostMsgSetLinkUnderCursor(ppapi::host::HostMessageContext* context,
                                      const std::string& url);

  content::RenderFrame* GetRenderFrame();

  content::RendererPpapiHost* const host_;

  DISALLOW_COPY_AND_ASSIGN(PepperPDFHost);
};

}  // namespace pdf

#endif  // COMPONENTS_PDF_RENDERER_PEPPER_PDF_HOST_H_
