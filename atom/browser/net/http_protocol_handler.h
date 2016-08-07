// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "net/url_request/url_request_job_factory.h"

namespace atom {

class HttpProtocolHandler : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  explicit HttpProtocolHandler(const std::string&);
  virtual ~HttpProtocolHandler();

  // net::URLRequestJobFactory::ProtocolHandler:
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  std::string scheme_;
};

}  // namespace atom
