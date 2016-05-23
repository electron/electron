// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BROWSER_DEVTOOLS_NETWORK_TRANSACTION_FACTORY_H_
#define BROWSER_DEVTOOLS_NETWORK_TRANSACTION_FACTORY_H_

#include "base/macros.h"
#include "net/base/request_priority.h"
#include "net/http/http_transaction_factory.h"

namespace brightray {

class DevToolsNetworkController;

class DevToolsNetworkTransactionFactory : public net::HttpTransactionFactory {
 public:
  explicit DevToolsNetworkTransactionFactory(
      DevToolsNetworkController* controller,
      net::HttpNetworkSession* session);
  ~DevToolsNetworkTransactionFactory() override;

  // net::HttpTransactionFactory:
  int CreateTransaction(net::RequestPriority priority,
                        std::unique_ptr<net::HttpTransaction>* transaction) override;
  net::HttpCache* GetCache() override;
  net::HttpNetworkSession* GetSession() override;

 private:
  DevToolsNetworkController*  controller_;
  std::unique_ptr<net::HttpTransactionFactory> network_layer_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsNetworkTransactionFactory);
};

}  // namespace brightray

#endif  // BROWSER_DEVTOOLS_NETWORK_TRANSACTION_FACTORY_H_
