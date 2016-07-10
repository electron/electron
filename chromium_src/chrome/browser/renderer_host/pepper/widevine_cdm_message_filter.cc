// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/pepper/widevine_cdm_message_filter.h"

#include "base/bind.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/webplugininfo.h"
#include "content/public/browser/plugin_service.h"

using content::PluginService;
using content::WebPluginInfo;
using content::BrowserThread;

WidevineCdmMessageFilter::WidevineCdmMessageFilter(
    int render_process_id,
    content::BrowserContext* browser_context)
    : BrowserMessageFilter(ChromeMsgStart),
      render_process_id_(render_process_id),
      browser_context_(browser_context) {
}

bool WidevineCdmMessageFilter::OnMessageReceived(const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(WidevineCdmMessageFilter, message)
#if defined(ENABLE_PEPPER_CDMS)
    IPC_MESSAGE_HANDLER(
        ChromeViewHostMsg_IsInternalPluginAvailableForMimeType,
        OnIsInternalPluginAvailableForMimeType)
#endif
    IPC_MESSAGE_UNHANDLED(return false)
  IPC_END_MESSAGE_MAP()
  return true;
}

#if defined(ENABLE_PEPPER_CDMS)
void WidevineCdmMessageFilter::OnIsInternalPluginAvailableForMimeType(
    const std::string& mime_type,
    bool* is_available,
    std::vector<base::string16>* additional_param_names,
    std::vector<base::string16>* additional_param_values) {
  std::vector<WebPluginInfo> plugins;
  PluginService::GetInstance()->GetInternalPlugins(&plugins);

  for (auto& plugin : plugins) {
    const std::vector<content::WebPluginMimeType>& mime_types =
        plugin.mime_types;
    for (const auto& j : mime_types) {
      if (j.mime_type == mime_type) {
        *is_available = true;
        *additional_param_names = j.additional_param_names;
        *additional_param_values = j.additional_param_values;
        return;
      }
    }
  }

  *is_available = false;
}
#endif // defined(ENABLE_PEPPER_CDMS)

void WidevineCdmMessageFilter::OnDestruct() const {
  BrowserThread::DeleteOnUIThread::Destruct(this);
}

WidevineCdmMessageFilter::~WidevineCdmMessageFilter() {
}
