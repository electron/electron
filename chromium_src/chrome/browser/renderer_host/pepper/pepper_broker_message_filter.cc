// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/pepper/pepper_broker_message_filter.h"

#include <string>

#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "ipc/ipc_message_macros.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "url/gurl.h"

using content::BrowserPpapiHost;
using content::BrowserThread;
using content::RenderProcessHost;

namespace chrome {

PepperBrokerMessageFilter::PepperBrokerMessageFilter(PP_Instance instance,
                                                     BrowserPpapiHost* host)
    : document_url_(host->GetDocumentURLForInstance(instance)) {
  int unused;
  host->GetRenderFrameIDsForInstance(instance, &render_process_id_, &unused);
}

PepperBrokerMessageFilter::~PepperBrokerMessageFilter() {}

scoped_refptr<base::TaskRunner>
PepperBrokerMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  return BrowserThread::GetMessageLoopProxyForThread(BrowserThread::UI);
}

int32_t PepperBrokerMessageFilter::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperBrokerMessageFilter, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_Broker_IsAllowed,
                                        OnIsAllowed)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperBrokerMessageFilter::OnIsAllowed(
    ppapi::host::HostMessageContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!document_url_.is_valid())
    return PP_ERROR_FAILED;
  RenderProcessHost* render_process_host =
      RenderProcessHost::FromID(render_process_id_);
  if (!render_process_host)
    return PP_ERROR_FAILED;
  Profile* profile =
      Profile::FromBrowserContext(render_process_host->GetBrowserContext());
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(profile);
  ContentSetting setting =
      content_settings->GetContentSetting(document_url_,
                                          document_url_,
                                          CONTENT_SETTINGS_TYPE_PPAPI_BROKER,
                                          std::string());
  if (setting == CONTENT_SETTING_ALLOW)
    return PP_OK;
  return PP_ERROR_FAILED;
}

}  // namespace chrome
