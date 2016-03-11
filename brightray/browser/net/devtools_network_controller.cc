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
    : default_interceptor_(new DevToolsNetworkInterceptor) {
}

DevToolsNetworkController::~DevToolsNetworkController() {
}

void DevToolsNetworkController::SetNetworkState(
    const std::string& client_id,
    scoped_ptr<DevToolsNetworkConditions> conditions) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (client_id.empty()) {
    if (!conditions)
      return;
    default_interceptor_->UpdateConditions(std::move(conditions));
    return;
  }

  auto interceptor = interceptors_.get(client_id);
  if (!interceptor) {
    if (!conditions)
      return;
    scoped_ptr<DevToolsNetworkInterceptor> new_interceptor(
        new DevToolsNetworkInterceptor);
    new_interceptor->UpdateConditions(std::move(conditions));
    interceptors_.set(client_id, std::move(new_interceptor));
  } else {
    if (!conditions) {
      scoped_ptr<DevToolsNetworkConditions> online_conditions(
          new DevToolsNetworkConditions(false));
      interceptor->UpdateConditions(std::move(online_conditions));
      interceptors_.erase(client_id);
    } else {
      interceptor->UpdateConditions(std::move(conditions));
    }
  }
}

base::WeakPtr<DevToolsNetworkInterceptor>
DevToolsNetworkController::GetInterceptor(DevToolsNetworkTransaction* transaction) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(transaction->request());

  if (!interceptors_.size())
    return default_interceptor_->GetWeakPtr();

  transaction->ProcessRequest();
  auto& client_id = transaction->client_id();
  auto interceptor = interceptors_.get(client_id);
  if (!interceptor)
    return default_interceptor_->GetWeakPtr();

  return interceptor->GetWeakPtr();
}

}  // namespace brightray
