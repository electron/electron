// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_URL_REQUEST_JOB_FACTORY_H_
#define ATOM_BROWSER_NET_ATOM_URL_REQUEST_JOB_FACTORY_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "net/url_request/url_request_job_factory.h"

namespace atom {

const void* DisableProtocolInterceptFlagKey();

class AtomURLRequestJobFactory : public net::URLRequestJobFactory {
 public:
  AtomURLRequestJobFactory();
  ~AtomURLRequestJobFactory() override;

  // Requests are forwarded to the chained job factory first.
  void Chain(std::unique_ptr<net::URLRequestJobFactory> job_factory);

  // Sets the ProtocolHandler for a scheme. Returns true on success, false on
  // failure (a ProtocolHandler already exists for |scheme|). On success,
  // URLRequestJobFactory takes ownership of |protocol_handler|.
  bool SetProtocolHandler(const std::string& scheme,
                          std::unique_ptr<ProtocolHandler> protocol_handler);

  // Intercepts the ProtocolHandler for a scheme.
  bool InterceptProtocol(const std::string& scheme,
                         std::unique_ptr<ProtocolHandler> protocol_handler);
  bool UninterceptProtocol(const std::string& scheme);

  // Whether the protocol handler is registered by the job factory.
  bool HasProtocolHandler(const std::string& scheme) const;

  // Clear all protocol handlers.
  void Clear();

  // URLRequestJobFactory implementation
  net::URLRequestJob* MaybeCreateJobWithProtocolHandler(
      const std::string& scheme,
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;
  net::URLRequestJob* MaybeInterceptRedirect(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      const GURL& location) const override;
  net::URLRequestJob* MaybeInterceptResponse(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;
  bool IsHandledProtocol(const std::string& scheme) const override;
  bool IsSafeRedirectTarget(const GURL& location) const override;

 private:
  using ProtocolHandlerMap = std::map<std::string, ProtocolHandler*>;

  ProtocolHandlerMap protocol_handler_map_;

  // Map that stores the original protocols of schemes.
  using OriginalProtocolsMap =
      std::unordered_map<std::string, std::unique_ptr<ProtocolHandler>>;
  // Can only be accessed in IO thread.
  OriginalProtocolsMap original_protocols_;

  std::unique_ptr<net::URLRequestJobFactory> job_factory_;

  DISALLOW_COPY_AND_ASSIGN(AtomURLRequestJobFactory);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_URL_REQUEST_JOB_FACTORY_H_
