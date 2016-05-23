// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/net/devtools_network_transaction_factory.h"

#include "browser/net/devtools_network_controller.h"
#include "browser/net/devtools_network_transaction.h"

#include "content/public/browser/service_worker_context.h"
#include "net/base/net_errors.h"
#include "net/http/http_network_layer.h"
#include "net/http/http_network_transaction.h"

namespace brightray {

DevToolsNetworkTransactionFactory::DevToolsNetworkTransactionFactory(
    DevToolsNetworkController* controller,
    net::HttpNetworkSession* session)
    : controller_(controller),
      network_layer_(new net::HttpNetworkLayer(session)) {
  std::set<std::string> headers;
  headers.insert(
      DevToolsNetworkTransaction::kDevToolsEmulateNetworkConditionsClientId);
  content::ServiceWorkerContext::AddExcludedHeadersForFetchEvent(headers);
}

DevToolsNetworkTransactionFactory::~DevToolsNetworkTransactionFactory() {
}

int DevToolsNetworkTransactionFactory::CreateTransaction(
    net::RequestPriority priority,
    std::unique_ptr<net::HttpTransaction>* transaction) {
  std::unique_ptr<net::HttpTransaction> new_transaction;
  int rv = network_layer_->CreateTransaction(priority, &new_transaction);
  if (rv != net::OK)
    return rv;
  transaction->reset(
      new DevToolsNetworkTransaction(controller_, std::move(new_transaction)));
  return net::OK;
}

net::HttpCache* DevToolsNetworkTransactionFactory::GetCache() {
  return network_layer_->GetCache();
}

net::HttpNetworkSession* DevToolsNetworkTransactionFactory::GetSession() {
  return network_layer_->GetSession();
}

}  // namespace brightray
