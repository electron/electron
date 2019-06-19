// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/about_protocol_handler.h"

#include "atom/browser/net/url_request_about_job.h"

namespace atom {

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

}  // namespace atom
