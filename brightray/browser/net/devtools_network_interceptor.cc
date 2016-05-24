// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/net/devtools_network_interceptor.h"

#include <algorithm>
#include <limits>

#include "base/time/time.h"
#include "browser/net/devtools_network_conditions.h"
#include "net/base/net_errors.h"

namespace brightray {

namespace {

int64_t kPacketSize = 1500;

base::TimeDelta CalculateTickLength(double throughput) {
  if (!throughput)
    return base::TimeDelta();

  int64_t us_tick_length = (1000000L * kPacketSize) / throughput;
  if (us_tick_length == 0)
    us_tick_length = 1;
  return base::TimeDelta::FromMicroseconds(us_tick_length);
}

}  // namespace

DevToolsNetworkInterceptor::ThrottleRecord::ThrottleRecord() {
}

DevToolsNetworkInterceptor::ThrottleRecord::ThrottleRecord(
    const ThrottleRecord& other) = default;

DevToolsNetworkInterceptor::ThrottleRecord::~ThrottleRecord() {
}

DevToolsNetworkInterceptor::DevToolsNetworkInterceptor()
    : conditions_(new DevToolsNetworkConditions(false)),
      download_last_tick_(0),
      upload_last_tick_(0),
      weak_ptr_factory_(this) {
}

DevToolsNetworkInterceptor::~DevToolsNetworkInterceptor() {
}

base::WeakPtr<DevToolsNetworkInterceptor>
DevToolsNetworkInterceptor::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void DevToolsNetworkInterceptor::FinishRecords(
    ThrottleRecords* records, bool offline) {
  ThrottleRecords temp;
  temp.swap(*records);
  for (const ThrottleRecord& record : temp) {
    bool failed = offline && !record.is_upload;
    record.callback.Run(
        failed ? net::ERR_INTERNET_DISCONNECTED : record.result,
        record.bytes);
  }
}

void DevToolsNetworkInterceptor::UpdateConditions(
    std::unique_ptr<DevToolsNetworkConditions> conditions) {
  DCHECK(conditions);
  base::TimeTicks now = base::TimeTicks::Now();
  if (conditions_->IsThrottling())
    UpdateThrottled(now);

  conditions_ = std::move(conditions);

  bool offline = conditions_->offline();
  if (offline || !conditions_->IsThrottling()) {
    timer_.Stop();
    FinishRecords(&download_, offline);
    FinishRecords(&upload_, offline);
    FinishRecords(&suspended_, offline);
    return;
  }

  // Throttling.
  DCHECK(conditions_->download_throughput() != 0 ||
         conditions_->upload_throughput() != 0);
  offset_ = now;

  download_last_tick_ = 0;
  download_tick_length_ = CalculateTickLength(
      conditions_->download_throughput());

  upload_last_tick_ = 0;
  upload_tick_length_ = CalculateTickLength(conditions_->upload_throughput());

  latency_length_ = base::TimeDelta();
  double latency = conditions_->latency();
  if (latency > 0)
    latency_length_ = base::TimeDelta::FromMillisecondsD(latency);
  ArmTimer(now);
}

uint64_t DevToolsNetworkInterceptor::UpdateThrottledRecords(
    base::TimeTicks now,
    ThrottleRecords* records,
    uint64_t last_tick,
    base::TimeDelta tick_length) {
  if (tick_length.is_zero()) {
    DCHECK(!records->size());
    return last_tick;
  }

  int64_t new_tick = (now - offset_) / tick_length;
  int64_t ticks = new_tick - last_tick;

  int64_t length = records->size();
  if (!length)
    return new_tick;

  int64_t shift = ticks % length;
  for (int64_t i = 0; i < length; ++i) {
    (*records)[i].bytes -=
        (ticks / length) * kPacketSize + (i < shift ? kPacketSize : 0);
  }
  std::rotate(records->begin(), records->begin() + shift, records->end());
  return new_tick;
}

void DevToolsNetworkInterceptor::UpdateThrottled(base::TimeTicks now) {
  download_last_tick_ = UpdateThrottledRecords(
      now, &download_, download_last_tick_, download_tick_length_);
  upload_last_tick_ = UpdateThrottledRecords(
      now, &upload_, upload_last_tick_, upload_tick_length_);
  UpdateSuspended(now);
}

void DevToolsNetworkInterceptor::UpdateSuspended(base::TimeTicks now) {
  int64_t activation_baseline =
      (now - latency_length_ - base::TimeTicks()).InMicroseconds();
  ThrottleRecords suspended;
  for (const ThrottleRecord& record : suspended_) {
    if (record.send_end <= activation_baseline) {
      if (record.is_upload)
        upload_.push_back(record);
      else
        download_.push_back(record);
    } else {
      suspended.push_back(record);
    }
  }
  suspended_.swap(suspended);
}

void DevToolsNetworkInterceptor::CollectFinished(
    ThrottleRecords* records, ThrottleRecords* finished) {
  ThrottleRecords active;
  for (const ThrottleRecord& record : *records) {
    if (record.bytes < 0)
      finished->push_back(record);
    else
      active.push_back(record);
  }
  records->swap(active);
}

void DevToolsNetworkInterceptor::OnTimer() {
  base::TimeTicks now = base::TimeTicks::Now();
  UpdateThrottled(now);

  ThrottleRecords finished;
  CollectFinished(&download_, &finished);
  CollectFinished(&upload_, &finished);
  for (const ThrottleRecord& record : finished)
    record.callback.Run(record.result, record.bytes);

  ArmTimer(now);
}

base::TimeTicks DevToolsNetworkInterceptor::CalculateDesiredTime(
    const ThrottleRecords& records,
    uint64_t last_tick,
    base::TimeDelta tick_length) {
  int64_t min_ticks_left = 0x10000L;
  size_t count = records.size();
  for (size_t i = 0; i < count; ++i) {
    int64_t packets_left = (records[i].bytes + kPacketSize - 1) / kPacketSize;
    int64_t ticks_left = (i + 1) + count * (packets_left - 1);
    if (i == 0 || ticks_left < min_ticks_left)
      min_ticks_left = ticks_left;
  }
  return offset_ + tick_length * (last_tick + min_ticks_left);
}

void DevToolsNetworkInterceptor::ArmTimer(base::TimeTicks now) {
  size_t suspend_count = suspended_.size();
  if (!download_.size() && !upload_.size() && !suspend_count)
    return;

  base::TimeTicks desired_time = CalculateDesiredTime(
      download_, download_last_tick_, download_tick_length_);

  base::TimeTicks upload_time = CalculateDesiredTime(
      upload_, upload_last_tick_, upload_tick_length_);
  if (upload_time < desired_time)
    desired_time = upload_time;

  int64_t min_baseline = std::numeric_limits<int64_t>::max();
  for (size_t i = 0; i < suspend_count; ++i) {
    if (suspended_[i].send_end < min_baseline)
      min_baseline = suspended_[i].send_end;
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

int DevToolsNetworkInterceptor::StartThrottle(
    int result,
    int64_t bytes,
    base::TimeTicks send_end,
    bool start,
    bool is_upload,
    const ThrottleCallback& callback) {
  if (result < 0)
    return result;

  if (conditions_->offline())
    return is_upload ? result : net::ERR_INTERNET_DISCONNECTED;

  if ((is_upload && !conditions_->upload_throughput()) ||
      (!is_upload && !conditions_->download_throughput())) {
    return result;
  }

  ThrottleRecord record;
  record.result = result;
  record.bytes = bytes;
  record.callback = callback;
  record.is_upload = is_upload;

  base::TimeTicks now = base::TimeTicks::Now();
  UpdateThrottled(now);
  if (start && latency_length_ != base::TimeDelta()) {
    record.send_end = (send_end - base::TimeTicks()).InMicroseconds();
    suspended_.push_back(record);
    UpdateSuspended(now);
  } else {
    if (is_upload)
      upload_.push_back(record);
    else
      download_.push_back(record);
  }
  ArmTimer(now);

  return net::ERR_IO_PENDING;
}

void DevToolsNetworkInterceptor::StopThrottle(
    const ThrottleCallback& callback) {
  RemoveRecord(&download_, callback);
  RemoveRecord(&upload_, callback);
  RemoveRecord(&suspended_, callback);
}

void DevToolsNetworkInterceptor::RemoveRecord(
    ThrottleRecords* records, const ThrottleCallback& callback) {
  records->erase(
      std::remove_if(records->begin(), records->end(),
                     [&callback](const ThrottleRecord& record){
                       return record.callback.Equals(callback);
                     }),
      records->end());
}

bool DevToolsNetworkInterceptor::IsOffline() {
  return conditions_->offline();
}

}  // namespace brightray
