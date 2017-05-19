// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brightray/browser/net/devtools_network_upload_data_stream.h"

#include "net/base/net_errors.h"

namespace brightray {

DevToolsNetworkUploadDataStream::DevToolsNetworkUploadDataStream(
    net::UploadDataStream* upload_data_stream)
    : net::UploadDataStream(upload_data_stream->is_chunked(),
                            upload_data_stream->identifier()),
      throttle_callback_(
          base::Bind(&DevToolsNetworkUploadDataStream::ThrottleCallback,
                     base::Unretained(this))),
      throttled_byte_count_(0),
      upload_data_stream_(upload_data_stream) {
}

DevToolsNetworkUploadDataStream::~DevToolsNetworkUploadDataStream() {
  if (interceptor_)
    interceptor_->StopThrottle(throttle_callback_);
}

void DevToolsNetworkUploadDataStream::SetInterceptor(
    DevToolsNetworkInterceptor* interceptor) {
  DCHECK(!interceptor_);
  if (interceptor)
    interceptor_ = interceptor->GetWeakPtr();
}

bool DevToolsNetworkUploadDataStream::IsInMemory() const {
  return false;
}

int DevToolsNetworkUploadDataStream::InitInternal(
    const net::NetLogWithSource& net_log) {
  throttled_byte_count_ = 0;
  int result = upload_data_stream_->Init(
      base::Bind(&DevToolsNetworkUploadDataStream::StreamInitCallback,
                 base::Unretained(this)),
      net_log);
  if (result == net::OK && !is_chunked())
    SetSize(upload_data_stream_->size());
  return result;
}

void DevToolsNetworkUploadDataStream::StreamInitCallback(int result) {
  if (!is_chunked())
    SetSize(upload_data_stream_->size());
  OnInitCompleted(result);
}

int DevToolsNetworkUploadDataStream::ReadInternal(
    net::IOBuffer* buf, int buf_len) {
  int result = upload_data_stream_->Read(buf, buf_len,
      base::Bind(&DevToolsNetworkUploadDataStream::StreamReadCallback,
                 base::Unretained(this)));
  return ThrottleRead(result);
}

void DevToolsNetworkUploadDataStream::StreamReadCallback(int result) {
  result = ThrottleRead(result);
  if (result != net::ERR_IO_PENDING)
    OnReadCompleted(result);
}

int DevToolsNetworkUploadDataStream::ThrottleRead(int result) {
  if (is_chunked() && upload_data_stream_->IsEOF())
    SetIsFinalChunk();

  if (!interceptor_ || result < 0)
    return result;

  if (result > 0)
    throttled_byte_count_ += result;
  return interceptor_->StartThrottle(result, throttled_byte_count_,
      base::TimeTicks(), false, true, throttle_callback_);
}

void DevToolsNetworkUploadDataStream::ThrottleCallback(
    int result, int64_t bytes) {
  throttled_byte_count_ = bytes;
  OnReadCompleted(result);
}

void DevToolsNetworkUploadDataStream::ResetInternal() {
  upload_data_stream_->Reset();
  throttled_byte_count_ = 0;
  if (interceptor_)
    interceptor_->StopThrottle(throttle_callback_);
}

}  // namespace brightray
