// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_url_request_job_factory.h"

#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;

namespace atom {

typedef net::URLRequestJobFactory::ProtocolHandler ProtocolHandler;

AtomURLRequestJobFactory::AtomURLRequestJobFactory() {}

AtomURLRequestJobFactory::~AtomURLRequestJobFactory() {
  STLDeleteValues(&protocol_handler_map_);
}

bool AtomURLRequestJobFactory::SetProtocolHandler(
    const std::string& scheme, scoped_ptr<ProtocolHandler> protocol_handler) {
  if (!protocol_handler) {
    ProtocolHandlerMap::iterator it = protocol_handler_map_.find(scheme);
    if (it == protocol_handler_map_.end())
      return false;

    delete it->second;
    protocol_handler_map_.erase(it);
    return true;
  }

  if (ContainsKey(protocol_handler_map_, scheme))
    return false;
  protocol_handler_map_[scheme] = protocol_handler.release();
  return true;
}

scoped_ptr<ProtocolHandler> AtomURLRequestJobFactory::ReplaceProtocol(
    const std::string& scheme, scoped_ptr<ProtocolHandler> protocol_handler) {
  if (!ContainsKey(protocol_handler_map_, scheme))
    return nullptr;
  ProtocolHandler* original_protocol_handler = protocol_handler_map_[scheme];
  protocol_handler_map_[scheme] = protocol_handler.release();
  return make_scoped_ptr(original_protocol_handler);
}

ProtocolHandler* AtomURLRequestJobFactory::GetProtocolHandler(
    const std::string& scheme) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ProtocolHandlerMap::const_iterator it = protocol_handler_map_.find(scheme);
  if (it == protocol_handler_map_.end())
    return nullptr;
  return it->second;
}

bool AtomURLRequestJobFactory::HasProtocolHandler(
    const std::string& scheme) const {
  return ContainsKey(protocol_handler_map_, scheme);
}

net::URLRequestJob* AtomURLRequestJobFactory::MaybeCreateJobWithProtocolHandler(
    const std::string& scheme,
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ProtocolHandlerMap::const_iterator it = protocol_handler_map_.find(scheme);
  if (it == protocol_handler_map_.end())
    return nullptr;
  return it->second->MaybeCreateJob(request, network_delegate);
}

net::URLRequestJob* AtomURLRequestJobFactory::MaybeInterceptRedirect(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const GURL& location) const {
  return nullptr;
}

net::URLRequestJob* AtomURLRequestJobFactory::MaybeInterceptResponse(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return nullptr;
}

bool AtomURLRequestJobFactory::IsHandledProtocol(
    const std::string& scheme) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  return HasProtocolHandler(scheme) ||
      net::URLRequest::IsHandledProtocol(scheme);
}

bool AtomURLRequestJobFactory::IsHandledURL(const GURL& url) const {
  if (!url.is_valid()) {
    // We handle error cases.
    return true;
  }
  return IsHandledProtocol(url.scheme());
}

bool AtomURLRequestJobFactory::IsSafeRedirectTarget(
    const GURL& location) const {
  return IsHandledURL(location);
}

}  // namespace atom
