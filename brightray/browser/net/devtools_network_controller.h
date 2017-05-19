// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_NET_DEVTOOLS_NETWORK_CONTROLLER_H_
#define BRIGHTRAY_BROWSER_NET_DEVTOOLS_NETWORK_CONTROLLER_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "base/threading/thread_checker.h"

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
  using InterceptorMap =
      std::unordered_map<std::string,
                         std::unique_ptr<DevToolsNetworkInterceptor>>;

  std::unique_ptr<DevToolsNetworkInterceptor> appcache_interceptor_;
  InterceptorMap interceptors_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsNetworkController);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_NET_DEVTOOLS_NETWORK_CONTROLLER_H_
