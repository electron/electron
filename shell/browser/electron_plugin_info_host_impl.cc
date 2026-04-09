// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/electron_plugin_info_host_impl.h"

#include <string>
#include <vector>

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/plugin_service.h"
#include "url/gurl.h"

using content::PluginService;
using content::WebPluginInfo;

namespace electron {

ElectronPluginInfoHostImpl::ElectronPluginInfoHostImpl() = default;

ElectronPluginInfoHostImpl::~ElectronPluginInfoHostImpl() = default;

void ElectronPluginInfoHostImpl::GetPluginInfo(const GURL& url,
                                               const url::Origin& origin,
                                               const std::string& mime_type,
                                               GetPluginInfoCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Ensure plugins are loaded (synchronous, non-blocking on UI thread).
  PluginService::GetInstance()->GetPlugins();

  mojom::PluginInfoPtr output = mojom::PluginInfo::New();
  std::vector<WebPluginInfo> matching_plugins;
  std::vector<std::string> mime_types;
  PluginService::GetInstance()->GetPluginInfoArray(
      url, mime_type, &matching_plugins, &mime_types);
  if (!matching_plugins.empty()) {
    output->plugin = matching_plugins[0];
    output->actual_mime_type = mime_types[0];
  }

  std::move(callback).Run(std::move(output));
}

}  // namespace electron
