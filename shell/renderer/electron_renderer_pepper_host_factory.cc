// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/renderer/electron_renderer_pepper_host_factory.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/ppapi_message_utils.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/ppapi_permissions.h"

using ppapi::host::ResourceHost;

// Stub class which ignores all messages
class PepperUMAHost : public ppapi::host::ResourceHost {
 public:
  PepperUMAHost(content::RendererPpapiHost* host,
                PP_Instance instance,
                PP_Resource resource)
      : ResourceHost(host->GetPpapiHost(), instance, resource) {}
  ~PepperUMAHost() override = default;

  // ppapi::host::ResourceMessageHandler implementation.
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override {
    PPAPI_BEGIN_MESSAGE_MAP(PepperUMAHost, msg)
      PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_UMA_HistogramCustomTimes,
                                        OnHistogramCustomTimes)
      PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_UMA_HistogramCustomCounts,
                                        OnHistogramCustomCounts)
      PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_UMA_HistogramEnumeration,
                                        OnHistogramEnumeration)
      PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
          PpapiHostMsg_UMA_IsCrashReportingEnabled, OnIsCrashReportingEnabled)
    PPAPI_END_MESSAGE_MAP()
    return PP_ERROR_FAILED;
  }

 private:
  int32_t OnHistogramCustomTimes(ppapi::host::HostMessageContext* context,
                                 const std::string& name,
                                 int64_t sample,
                                 int64_t min,
                                 int64_t max,
                                 uint32_t bucket_count) {
    return PP_OK;
  }

  int32_t OnHistogramCustomCounts(ppapi::host::HostMessageContext* context,
                                  const std::string& name,
                                  int32_t sample,
                                  int32_t min,
                                  int32_t max,
                                  uint32_t bucket_count) {
    return PP_OK;
  }

  int32_t OnHistogramEnumeration(ppapi::host::HostMessageContext* context,
                                 const std::string& name,
                                 int32_t sample,
                                 int32_t boundary_value) {
    return PP_OK;
  }

  int32_t OnIsCrashReportingEnabled(ppapi::host::HostMessageContext* context) {
    return PP_OK;
  }
};

ElectronRendererPepperHostFactory::ElectronRendererPepperHostFactory(
    content::RendererPpapiHost* host)
    : host_(host) {}

ElectronRendererPepperHostFactory::~ElectronRendererPepperHostFactory() =
    default;

std::unique_ptr<ResourceHost>
ElectronRendererPepperHostFactory::CreateResourceHost(
    ppapi::host::PpapiHost* host,
    PP_Resource resource,
    PP_Instance instance,
    const IPC::Message& message) {
  DCHECK_EQ(host_->GetPpapiHost(), host);

  // Make sure the plugin is giving us a valid instance for this resource.
  if (!host_->IsValidInstance(instance))
    return nullptr;

  // Permissions for the following interfaces will be checked at the
  // time of the corresponding instance's method calls.  Currently these
  // interfaces are available only for specifically permitted apps which may
  // not have access to the other private interfaces.
  switch (message.type()) {
    case PpapiHostMsg_UMA_Create::ID: {
      return std::make_unique<PepperUMAHost>(host_, instance, resource);
    }
  }

  return nullptr;
}
