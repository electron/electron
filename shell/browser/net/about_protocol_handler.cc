// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/about_protocol_handler.h"

#include "shell/browser/net/url_request_about_job.h"

namespace electron {

AboutProtocolHandler::AboutProtocolHandler() {}

AboutProtocolHandler::~AboutProtocolHandler() {}

net::URLRequestJob* AboutProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return new URLRequestAboutJob(request, network_delegate);
}

bool AboutProtocolHandler::IsSafeRedirectTarget(const GURL& location) const {
  return false;
}

}  // namespace electron
