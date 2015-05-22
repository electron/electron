// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_buffer_job.h"

#include <string>

#include "net/base/net_errors.h"

namespace atom {

URLRequestBufferJob::URLRequestBufferJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const std::string& mime_type,
    const std::string& charset,
    scoped_refptr<base::RefCountedBytes> data)
    : net::URLRequestSimpleJob(request, network_delegate),
      mime_type_(mime_type),
      charset_(charset),
      buffer_data_(data) {
}

int URLRequestBufferJob::GetRefCountedData(
    std::string* mime_type,
    std::string* charset,
    scoped_refptr<base::RefCountedMemory>* data,
    const net::CompletionCallback& callback) const {
  *mime_type = mime_type_;
  *charset = charset_;
  *data = buffer_data_;
  return net::OK;
}

}  // namespace atom
