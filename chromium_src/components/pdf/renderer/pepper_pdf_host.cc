// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/pdf/renderer/pepper_pdf_host.h"

#include "base/memory/ptr_util.h"
#include "components/pdf/common/pdf_messages.h"
#include "content/public/common/referrer.h"
#include "content/public/renderer/pepper_plugin_instance.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace pdf {

PepperPDFHost::PepperPDFHost(content::RendererPpapiHost* host,
                             PP_Instance instance,
                             PP_Resource resource)
    : ppapi::host::ResourceHost(host->GetPpapiHost(), instance, resource),
      host_(host) {}

PepperPDFHost::~PepperPDFHost() {}

int32_t PepperPDFHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperPDFHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_PDF_DidStartLoading,
                                        OnHostMsgDidStartLoading)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_PDF_DidStopLoading,
                                        OnHostMsgDidStopLoading)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_PDF_SaveAs,
                                        OnHostMsgSaveAs)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_PDF_SetSelectedText,
                                      OnHostMsgSetSelectedText)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_PDF_SetLinkUnderCursor,
                                      OnHostMsgSetLinkUnderCursor)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperPDFHost::OnHostMsgDidStartLoading(
    ppapi::host::HostMessageContext* context) {
  content::RenderFrame* render_frame = GetRenderFrame();
  if (!render_frame)
    return PP_ERROR_FAILED;

  render_frame->PluginDidStartLoading();
  return PP_OK;
}

int32_t PepperPDFHost::OnHostMsgDidStopLoading(
    ppapi::host::HostMessageContext* context) {
  content::RenderFrame* render_frame = GetRenderFrame();
  if (!render_frame)
    return PP_ERROR_FAILED;

  render_frame->PluginDidStopLoading();
  return PP_OK;
}

int32_t PepperPDFHost::OnHostMsgSaveAs(
    ppapi::host::HostMessageContext* context) {
  content::PepperPluginInstance* instance =
      host_->GetPluginInstance(pp_instance());
  if (!instance)
    return PP_ERROR_FAILED;

  content::RenderFrame* render_frame = instance->GetRenderFrame();
  if (!render_frame)
    return PP_ERROR_FAILED;

  GURL url = instance->GetPluginURL();
  content::Referrer referrer;
  referrer.url = url;
  referrer.policy = blink::kWebReferrerPolicyDefault;
  referrer = content::Referrer::SanitizeForRequest(url, referrer);
  render_frame->Send(
      new PDFHostMsg_PDFSaveURLAs(render_frame->GetRoutingID(), url, referrer));
  return PP_OK;
}

int32_t PepperPDFHost::OnHostMsgSetSelectedText(
    ppapi::host::HostMessageContext* context,
    const base::string16& selected_text) {
  content::PepperPluginInstance* instance =
      host_->GetPluginInstance(pp_instance());
  if (!instance)
    return PP_ERROR_FAILED;
  instance->SetSelectedText(selected_text);
  return PP_OK;
}

int32_t PepperPDFHost::OnHostMsgSetLinkUnderCursor(
    ppapi::host::HostMessageContext* context,
    const std::string& url) {
  content::PepperPluginInstance* instance =
      host_->GetPluginInstance(pp_instance());
  if (!instance)
    return PP_ERROR_FAILED;
  instance->SetLinkUnderCursor(url);
  return PP_OK;
}

content::RenderFrame* PepperPDFHost::GetRenderFrame() {
  content::PepperPluginInstance* instance =
      host_->GetPluginInstance(pp_instance());
  return instance ? instance->GetRenderFrame() : nullptr;
}

}  // namespace pdf
