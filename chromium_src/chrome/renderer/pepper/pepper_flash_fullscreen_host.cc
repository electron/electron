// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/pepper/pepper_flash_fullscreen_host.h"

#include "content/public/renderer/pepper_plugin_instance.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"

PepperFlashFullscreenHost::PepperFlashFullscreenHost(
    content::RendererPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      renderer_ppapi_host_(host) {}

PepperFlashFullscreenHost::~PepperFlashFullscreenHost() {}

int32_t PepperFlashFullscreenHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperFlashFullscreenHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_FlashFullscreen_SetFullscreen,
      OnSetFullscreen)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperFlashFullscreenHost::OnSetFullscreen(
    ppapi::host::HostMessageContext* context,
    bool fullscreen) {
  content::PepperPluginInstance* plugin_instance =
      renderer_ppapi_host_->GetPluginInstance(pp_instance());
  if (plugin_instance && plugin_instance->FlashSetFullscreen(fullscreen, true))
    return PP_OK;
  return PP_ERROR_FAILED;
}
