// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/net/devtools_network_interceptor.h"

#include <limits>

#include "browser/net/devtools_network_conditions.h"
#include "browser/net/devtools_network_transaction.h"

#include "base/time/time.h"
#include "net/base/load_timing_info.h"

namespace brightray {

namespace {

int64_t kPacketSize = 1500;

}  // namespace

DevToolsNetworkInterceptor::DevToolsNetworkInterceptor()
    : conditions_(new DevToolsNetworkConditions(false)),
      weak_ptr_factory_(this) {
}

DevToolsNetworkInterceptor::~DevToolsNetworkInterceptor() {
}

base::WeakPtr<DevToolsNetworkInterceptor>
DevToolsNetworkInterceptor::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void DevToolsNetworkInterceptor::UpdateConditions(
    scoped_ptr<DevToolsNetworkConditions> conditions) {
  DCHECK(conditions);
  base::TimeTicks now = base::TimeTicks::Now();
  if (conditions_->IsThrottling())
    UpdateThrottledTransactions(now);

  conditions_ = conditions.Pass();

  if (conditions_->offline()) {
    timer_.Stop();
    throttled_transactions_.clear();
    suspended_transactions_.clear();
    Transactions old_transactions(transactions_);
    Transactions::iterator it = old_transactions.begin();
    for (; it != old_transactions.end(); ++it) {
      if (transactions_.find(*it) == transactions_.end())
        continue;
      if (!(*it)->request() || (*it)->failed())
        continue;
      if (ShouldFail(*it))
        (*it)->Fail();
    }
    return;
  }

  if (conditions_->IsThrottling()) {
    DCHECK_NE(conditions_->download_throughput(), 0);
    offset_ = now;
    last_tick_ = 0;
    int64_t us_tick_length =
        (1000000L * kPacketSize) / conditions_->download_throughput();
    DCHECK_NE(us_tick_length, 0);
    if (us_tick_length == 0)
      us_tick_length = 1;
    tick_length_ = base::TimeDelta::FromMicroseconds(us_tick_length);
    latency_length_ = base::TimeDelta();
    double latency = conditions_->latency();
    if (latency > 0)
      latency_length_ = base::TimeDelta::FromMillisecondsD(latency);
    ArmTimer(now);
  } else {
    timer_.Stop();

    std::vector<DevToolsNetworkTransaction*> throttled_transactions;
    throttled_transactions.swap(throttled_transactions_);
    for (auto& throttled_transaction : throttled_transactions)
      FireThrottledCallback(throttled_transaction);

    SuspendedTransactions suspended_transactions;
    suspended_transactions.swap(suspended_transactions_);
    for (auto& suspended_transaction : suspended_transactions)
      FireThrottledCallback(suspended_transaction.first);
  }
}

void DevToolsNetworkInterceptor::AddTransaction(
    DevToolsNetworkTransaction* transaction) {
  DCHECK(transactions_.find(transaction) == transactions_.end());
  transactions_.insert(transaction);
}

void DevToolsNetworkInterceptor::RemoveTransaction(
    DevToolsNetworkTransaction* transaction) {
  DCHECK(transactions_.find(transaction) != transactions_.end());
  transactions_.erase(transaction);

  if (!conditions_->IsThrottling())
    return;

  base::TimeTicks now = base::TimeTicks::Now();
  UpdateThrottledTransactions(now);
  throttled_transactions_.erase(std::remove(throttled_transactions_.begin(),
      throttled_transactions_.end(), transaction),
      throttled_transactions_.end());

  SuspendedTransactions::iterator it = suspended_transactions_.begin();
  for (; it != suspended_transactions_.end(); ++it) {
    if (it->first == transaction) {
      suspended_transactions_.erase(it);
      break;
    }
  }

  ArmTimer(now);
}

bool DevToolsNetworkInterceptor::ShouldFail(
    const DevToolsNetworkTransaction* transaction) {
  return conditions_->offline();
}

bool DevToolsNetworkInterceptor::ShouldThrottle(
    const DevToolsNetworkTransaction* transaction) {
  return conditions_->IsThrottling();
}

void DevToolsNetworkInterceptor::ThrottleTransaction(
    DevToolsNetworkTransaction* transaction, bool start) {
  base::TimeTicks now = base::TimeTicks::Now();
  UpdateThrottledTransactions(now);
  if (start && latency_length_ != base::TimeDelta()) {
    net::LoadTimingInfo load_timing_info;
    base::TimeTicks send_end;
    if (transaction->GetLoadTimingInfo(&load_timing_info))
      send_end = load_timing_info.send_end;
    if (send_end.is_null())
      send_end = now;
    int64_t us_send_end = (send_end - base::TimeTicks()).InMicroseconds();
    suspended_transactions_.push_back(
        SuspendedTransaction(transaction, us_send_end));
    UpdateSuspendedTransactions(now);
  } else {
    throttled_transactions_.push_back(transaction);
  }
  ArmTimer(now);
}

void DevToolsNetworkInterceptor::UpdateThrottledTransactions(
    base::TimeTicks now) {
  int64_t last_tick = (now - offset_) / tick_length_;
  int64_t ticks = last_tick - last_tick_;
  last_tick_ = last_tick;

  int64_t length = throttled_transactions_.size();
  if (!length) {
    UpdateSuspendedTransactions(now);
    return;
  }

  int64_t shift = ticks % length;
  for (int64_t i = 0; i < length; ++i) {
    throttled_transactions_[i]->DecreaseThrottledByteCount(
        (ticks / length) * kPacketSize + (i < shift ? kPacketSize : 0));
  }
  std::rotate(throttled_transactions_.begin(),
      throttled_transactions_.begin() + shift, throttled_transactions_.end());

  UpdateSuspendedTransactions(now);
}

void DevToolsNetworkInterceptor::UpdateSuspendedTransactions(
    base::TimeTicks now) {
  int64_t activation_baseline =
      (now - latency_length_ - base::TimeTicks()).InMicroseconds();
  SuspendedTransactions suspended_transactions;
  SuspendedTransactions::iterator it = suspended_transactions_.begin();
  for (; it != suspended_transactions_.end(); ++it) {
    if (it->second <= activation_baseline)
      throttled_transactions_.push_back(it->first);
    else
      suspended_transactions.push_back(*it);
  }
  suspended_transactions_.swap(suspended_transactions);
}

void DevToolsNetworkInterceptor::ArmTimer(base::TimeTicks now) {
  size_t throttle_count = throttled_transactions_.size();
  size_t suspend_count = suspended_transactions_.size();
  if (!throttle_count && !suspend_count)
    return;

  int64_t min_ticks_left = 0x10000L;
  for (size_t i = 0; i < throttle_count; ++i) {
    int64_t packets_left = (throttled_transactions_[i]->throttled_byte_count() +
        kPacketSize - 1) / kPacketSize;
    int64_t ticks_left = (i + 1) + throttle_count * (packets_left - 1);
    if (i == 0 || ticks_left < min_ticks_left)
      min_ticks_left = ticks_left;
  }

  base::TimeTicks desired_time =
      offset_ + tick_length_ * (last_tick_ + min_ticks_left);

  int64_t min_baseline = std::numeric_limits<int64>::max();
  for (size_t i = 0; i < suspend_count; ++i) {
    if (suspended_transactions_[i].second < min_baseline)
      min_baseline = suspended_transactions_[i].second;
  }

  if (suspend_count) {
    base::TimeTicks activation_time = base::TimeTicks() +
        base::TimeDelta::FromMicroseconds(min_baseline) + latency_length_;
    if (activation_time < desired_time)
      desired_time = activation_time;
  }

  timer_.Start(FROM_HERE, desired_time - now,
      base::Bind(&DevToolsNetworkInterceptor::OnTimer,
                 base::Unretained(this)));
}

void DevToolsNetworkInterceptor::OnTimer() {
  base::TimeTicks now = base::TimeTicks::Now();
  UpdateThrottledTransactions(now);

  std::vector<DevToolsNetworkTransaction*> active_transactions;
  std::vector<DevToolsNetworkTransaction*> finished_transactions;
  size_t length = throttled_transactions_.size();
  for (size_t i = 0; i < length; ++i) {
    if (throttled_transactions_[i]->throttled_byte_count() < 0)
      finished_transactions.push_back(throttled_transactions_[i]);
    else
      active_transactions.push_back(throttled_transactions_[i]);
  }
  throttled_transactions_.swap(active_transactions);

  for (auto& transaction : finished_transactions)
    FireThrottledCallback(transaction);

  ArmTimer(now);
}

void DevToolsNetworkInterceptor::FireThrottledCallback(
    DevToolsNetworkTransaction* transaction) {
  if (transactions_.find(transaction) != transactions_.end())
    transaction->FireThrottledCallback();
}

}  // namespace brightray
