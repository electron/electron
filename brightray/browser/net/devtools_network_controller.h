// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BROWSER_DEVTOOLS_NETWORK_CONTROLLER_H_
#define BROWSER_DEVTOOLS_NETWORK_CONTROLLER_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"

namespace brightray {

class DevToolsNetworkConditions;
class DevToolsNetworkInterceptor;
class DevToolsNetworkTransaction;

class DevToolsNetworkController {
 public:
  DevToolsNetworkController();
  virtual ~DevToolsNetworkController();

  void SetNetworkState(const std::string& client_id,
                       scoped_ptr<DevToolsNetworkConditions> conditions);
  base::WeakPtr<DevToolsNetworkInterceptor> GetInterceptor(
      DevToolsNetworkTransaction* transaction);

 private:
  using InterceptorMap = base::ScopedPtrHashMap<std::string,
                                                scoped_ptr<DevToolsNetworkInterceptor>>;
  scoped_ptr<DevToolsNetworkInterceptor> default_interceptor_;
  InterceptorMap interceptors_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsNetworkController);
};

}  // namespace brightray

#endif  // BROWSER_DEVTOOLS_NETWORK_CONTROLLER_H_
