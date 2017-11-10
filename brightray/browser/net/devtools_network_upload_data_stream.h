// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_NET_DEVTOOLS_NETWORK_UPLOAD_DATA_STREAM_H_
#define BRIGHTRAY_BROWSER_NET_DEVTOOLS_NETWORK_UPLOAD_DATA_STREAM_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "brightray/browser/net/devtools_network_interceptor.h"
#include "net/base/upload_data_stream.h"

namespace brightray {

class DevToolsNetworkUploadDataStream : public net::UploadDataStream {
 public:
  // Supplied |upload_data_stream| must outlive this object.
  explicit DevToolsNetworkUploadDataStream(
      net::UploadDataStream* upload_data_stream);
  ~DevToolsNetworkUploadDataStream() override;

  void SetInterceptor(DevToolsNetworkInterceptor* interceptor);

 private:
  // net::UploadDataStream implementation.
  bool IsInMemory() const override;
  int InitInternal(const net::NetLogWithSource& net_log) override;
  int ReadInternal(net::IOBuffer* buf, int buf_len) override;
  void ResetInternal() override;

  void StreamInitCallback(int result);
  void StreamReadCallback(int result);

  int ThrottleRead(int result);
  void ThrottleCallback(int result, int64_t bytes);

  DevToolsNetworkInterceptor::ThrottleCallback throttle_callback_;
  int64_t throttled_byte_count_;

  net::UploadDataStream* upload_data_stream_;
  base::WeakPtr<DevToolsNetworkInterceptor> interceptor_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsNetworkUploadDataStream);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_NET_DEVTOOLS_NETWORK_UPLOAD_DATA_STREAM_H_
