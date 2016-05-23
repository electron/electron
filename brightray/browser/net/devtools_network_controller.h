// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BROWSER_DEVTOOLS_NETWORK_CONTROLLER_H_
#define BROWSER_DEVTOOLS_NETWORK_CONTROLLER_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/macros.h"

namespace brightray {

class DevToolsNetworkConditions;
class DevToolsNetworkInterceptor;
class DevToolsNetworkTransaction;

class DevToolsNetworkController {
 public:
  DevToolsNetworkController();
  virtual ~DevToolsNetworkController();

  void SetNetworkState(const std::string& client_id,
                       std::unique_ptr<DevToolsNetworkConditions> conditions);
  DevToolsNetworkInterceptor* GetInterceptor(const std::string& client_id);

 private:
  using InterceptorMap = base::ScopedPtrHashMap<std::string,
                                                std::unique_ptr<DevToolsNetworkInterceptor>>;

  std::unique_ptr<DevToolsNetworkInterceptor> appcache_interceptor_;
  InterceptorMap interceptors_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsNetworkController);
};

}  // namespace brightray

#endif  // BROWSER_DEVTOOLS_NETWORK_CONTROLLER_H_
