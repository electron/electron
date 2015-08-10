// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/ftp_protocol_handler.h"

#include "net/url_request/url_request_context.h"
#include "net/ftp/ftp_network_layer.h"

namespace atom {

FtpProtocolHandler::FtpProtocolHandler() {
}

FtpProtocolHandler::~FtpProtocolHandler() {
}

net::URLRequestJob* FtpProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  if (!orig_protocol_handler_) {
    auto host_resolver = request->context()->host_resolver();
    orig_protocol_handler_.reset(new net::FtpProtocolHandler(
        new net::FtpNetworkLayer(host_resolver)));
  }
  return orig_protocol_handler_->MaybeCreateJob(request, network_delegate);
}

}  // namespace atom
