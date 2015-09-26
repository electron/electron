// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_DEVTOOLS_NETWORK_INTERCEPTOR_H_
#define BROWSER_DEVTOOLS_NETWORK_INTERCEPTOR_H_

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/timer/timer.h"

namespace base {
class TimeDelta;
class TimeTicks;
}

namespace brightray {

class DevToolsNetworkConditions;
class DevToolsNetworkTransaction;

class DevToolsNetworkInterceptor {
 public:
  DevToolsNetworkInterceptor();
  virtual ~DevToolsNetworkInterceptor();

  base::WeakPtr<DevToolsNetworkInterceptor> GetWeakPtr();

  // Applies network emulation configuration.
  void UpdateConditions(scoped_ptr<DevToolsNetworkConditions> conditions);

  void AddTransaction(DevToolsNetworkTransaction* transaction);
  void RemoveTransaction(DevToolsNetworkTransaction* transaction);

  // Returns whether transaction should fail with |net::ERR_INTERNET_DISCONNECTED|
  bool ShouldFail(const DevToolsNetworkTransaction* transaction);
  // Returns whether transaction should be throttled.
  bool ShouldThrottle(const DevToolsNetworkTransaction* transaction);

  void ThrottleTransaction(DevToolsNetworkTransaction* transaction, bool start);

  const DevToolsNetworkConditions* conditions() const {
    return conditions_.get();
  }

 private:
  void UpdateThrottledTransactions(base::TimeTicks now);
  void UpdateSuspendedTransactions(base::TimeTicks now);
  void ArmTimer(base::TimeTicks now);
  void OnTimer();
  void FireThrottledCallback(DevToolsNetworkTransaction* transaction);

  scoped_ptr<DevToolsNetworkConditions> conditions_;

  using Transactions = std::set<DevToolsNetworkTransaction*>;
  Transactions transactions_;

  // Transactions suspended for a latency period.
  using SuspendedTransaction = std::pair<DevToolsNetworkTransaction*, int64_t>;
  using SuspendedTransactions = std::vector<SuspendedTransaction>;
  SuspendedTransactions suspended_transactions_;

  // Transactions waiting certain amount of transfer to be accounted.
  std::vector<DevToolsNetworkTransaction*> throttled_transactions_;

  base::OneShotTimer<DevToolsNetworkInterceptor> timer_;
  base::TimeTicks offset_;
  base::TimeDelta tick_length_;
  base::TimeDelta latency_length_;
  uint64_t last_tick_;

  base::WeakPtrFactory<DevToolsNetworkInterceptor> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsNetworkInterceptor);
};

}  // namespace brightray

#endif  // BROWSER_DEVTOOLS_NETWORK_INTERCEPTOR_H_
