// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/net/devtools_network_controller.h"

#include "browser/net/devtools_network_conditions.h"
#include "browser/net/devtools_network_interceptor.h"
#include "browser/net/devtools_network_transaction.h"

#include "base/bind.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace brightray {

DevToolsNetworkController::DevToolsNetworkController()
    : appcache_interceptor_(new DevToolsNetworkInterceptor) {
}

DevToolsNetworkController::~DevToolsNetworkController() {
}

void DevToolsNetworkController::SetNetworkState(
    const std::string& client_id,
    std::unique_ptr<DevToolsNetworkConditions> conditions) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  DevToolsNetworkInterceptor* interceptor = interceptors_.get(client_id);
  if (!interceptor) {
    if (!conditions)
      return;
    std::unique_ptr<DevToolsNetworkInterceptor> new_interceptor(
        new DevToolsNetworkInterceptor);
    new_interceptor->UpdateConditions(std::move(conditions));
    interceptors_.set(client_id, std::move(new_interceptor));
  } else {
    if (!conditions) {
      std::unique_ptr<DevToolsNetworkConditions> online_conditions(
          new DevToolsNetworkConditions(false));
      interceptor->UpdateConditions(std::move(online_conditions));
      interceptors_.erase(client_id);
    } else {
      interceptor->UpdateConditions(std::move(conditions));
    }
  }

  bool has_offline_interceptors = false;
  auto it = interceptors_.begin();
  for (; it != interceptors_.end(); ++it) {
    if (it->second->IsOffline()) {
      has_offline_interceptors = true;
      break;
    }
  }

  bool is_appcache_offline = appcache_interceptor_->IsOffline();
  if (is_appcache_offline != has_offline_interceptors) {
    std::unique_ptr<DevToolsNetworkConditions> appcache_conditions(
        new DevToolsNetworkConditions(has_offline_interceptors));
    appcache_interceptor_->UpdateConditions(std::move(appcache_conditions));
  }
}

DevToolsNetworkInterceptor*
DevToolsNetworkController::GetInterceptor(const std::string& client_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!interceptors_.size() || client_id.empty())
    return nullptr;

  DevToolsNetworkInterceptor* interceptor = interceptors_.get(client_id);
  if (!interceptor)
    return nullptr;

  return interceptor;
}

}  // namespace brightray
