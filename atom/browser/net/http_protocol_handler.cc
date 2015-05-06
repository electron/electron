// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/http_protocol_handler.h"

#include "net/url_request/url_request_http_job.h"

namespace atom {

HttpProtocolHandler::HttpProtocolHandler() {
}

HttpProtocolHandler::~HttpProtocolHandler() {
}

net::URLRequestJob* HttpProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return net::URLRequestHttpJob::Factory(request,
                                         network_delegate,
                                         "http");
}

}  // namespace atom
