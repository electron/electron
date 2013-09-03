// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/net/url_request_string_job.h"

#include "net/base/net_errors.h"

namespace atom {

URLRequestStringJob::URLRequestStringJob(net::URLRequest* request,
                                         net::NetworkDelegate* network_delegate,
                                         const std::string& mime_type,
                                         const std::string& charset,
                                         const std::string& data)
    : net::URLRequestSimpleJob(request, network_delegate),
      mime_type_(mime_type),
      charset_(charset),
      data_(data) {
}

int URLRequestStringJob::GetData(
    std::string* mime_type,
    std::string* charset,
    std::string* data,
    const net::CompletionCallback& callback) const {
  *mime_type = mime_type_;
  *charset = charset_;
  *data = data_;
  return net::OK;
}

}  // namespace atom
