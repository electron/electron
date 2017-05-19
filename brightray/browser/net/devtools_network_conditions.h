// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_NET_DEVTOOLS_NETWORK_CONDITIONS_H_
#define BRIGHTRAY_BROWSER_NET_DEVTOOLS_NETWORK_CONDITIONS_H_

#include "base/macros.h"

namespace brightray {

class DevToolsNetworkConditions {
 public:
  explicit DevToolsNetworkConditions(bool offline);
  DevToolsNetworkConditions(bool offline,
                            double latency,
                            double download_throughput,
                            double upload_throughput);
  ~DevToolsNetworkConditions();

  bool IsThrottling() const;

  bool offline() const { return offline_; }
  double latency() const { return latency_; }
  double download_throughput() const { return download_throughput_; }
  double upload_throughput() const { return upload_throughput_; }

 private:
  const bool offline_;
  const double latency_;
  const double download_throughput_;
  const double upload_throughput_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsNetworkConditions);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_NET_DEVTOOLS_NETWORK_CONDITIONS_H_
