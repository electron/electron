// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NET_ABOUT_PROTOCOL_HANDLER_H_
#define SHELL_BROWSER_NET_ABOUT_PROTOCOL_HANDLER_H_

#include "net/url_request/url_request_job_factory.h"

namespace electron {

class AboutProtocolHandler : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  AboutProtocolHandler();
  ~AboutProtocolHandler() override;

  // net::URLRequestJobFactory::ProtocolHandler:
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;
  bool IsSafeRedirectTarget(const GURL& location) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AboutProtocolHandler);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NET_ABOUT_PROTOCOL_HANDLER_H_
