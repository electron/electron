// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brightray/browser/net/devtools_network_controller.h"

#include "base/bind.h"
#include "brightray/browser/net/devtools_network_conditions.h"
#include "brightray/browser/net/devtools_network_interceptor.h"
#include "brightray/browser/net/devtools_network_transaction.h"
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

  auto it = interceptors_.find(client_id);
  if (it == interceptors_.end()) {
    if (!conditions)
      return;
    std::unique_ptr<DevToolsNetworkInterceptor> new_interceptor(
        new DevToolsNetworkInterceptor);
    new_interceptor->UpdateConditions(std::move(conditions));
    interceptors_[client_id] = std::move(new_interceptor);
  } else {
    if (!conditions) {
      std::unique_ptr<DevToolsNetworkConditions> online_conditions(
          new DevToolsNetworkConditions(false));
      it->second->UpdateConditions(std::move(online_conditions));
      interceptors_.erase(client_id);
    } else {
      it->second->UpdateConditions(std::move(conditions));
    }
  }

  bool has_offline_interceptors = false;
  for (const auto& interceptor : interceptors_) {
    if (interceptor.second->IsOffline()) {
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

  if (interceptors_.empty() || client_id.empty())
    return nullptr;

  auto it = interceptors_.find(client_id);
  if (it == interceptors_.end())
    return nullptr;

  return it->second.get();
}

}  // namespace brightray
