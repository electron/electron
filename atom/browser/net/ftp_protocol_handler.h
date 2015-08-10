// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_FTP_PROTOCOL_HANDLER_H_
#define ATOM_BROWSER_NET_FTP_PROTOCOL_HANDLER_H_

#include <string>

#include "net/url_request/url_request_job_factory.h"
#include "net/url_request/ftp_protocol_handler.h"
#include "base/memory/scoped_ptr.h"

namespace atom {

class FtpProtocolHandler : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  FtpProtocolHandler();
  virtual ~FtpProtocolHandler();

  // net::URLRequestJobFactory::ProtocolHandler:
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  mutable scoped_ptr<net::FtpProtocolHandler> orig_protocol_handler_;
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_FTP_PROTOCOL_HANDLER_H_
