// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/net/atom_url_request_job_factory.h"

#include "base/stl_util.h"
#include "googleurl/src/gurl.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_request.h"

namespace atom {

typedef net::URLRequestJobFactory::ProtocolHandler ProtocolHandler;

AtomURLRequestJobFactory::AtomURLRequestJobFactory() {}

AtomURLRequestJobFactory::~AtomURLRequestJobFactory() {
  STLDeleteValues(&protocol_handler_map_);
}

bool AtomURLRequestJobFactory::SetProtocolHandler(
    const std::string& scheme,
    ProtocolHandler* protocol_handler) {
  DCHECK(CalledOnValidThread());

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
  protocol_handler_map_[scheme] = protocol_handler;
  return true;
}

ProtocolHandler* AtomURLRequestJobFactory::InterceptProtocol(
    const std::string& scheme,
    ProtocolHandler* protocol_handler) {
  DCHECK(CalledOnValidThread());
  DCHECK(protocol_handler);

  if (!ContainsKey(protocol_handler_map_, scheme))
    return NULL;
  ProtocolHandler* original_protocol_handler = protocol_handler_map_[scheme];
  protocol_handler_map_[scheme] = protocol_handler;
  return original_protocol_handler;
}

net::URLRequestJob* AtomURLRequestJobFactory::MaybeCreateJobWithProtocolHandler(
    const std::string& scheme,
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  DCHECK(CalledOnValidThread());
  ProtocolHandlerMap::const_iterator it = protocol_handler_map_.find(scheme);
  if (it == protocol_handler_map_.end())
    return NULL;
  return it->second->MaybeCreateJob(request, network_delegate);
}

bool AtomURLRequestJobFactory::IsHandledProtocol(
    const std::string& scheme) const {
  DCHECK(CalledOnValidThread());
  return ContainsKey(protocol_handler_map_, scheme) ||
      net::URLRequest::IsHandledProtocol(scheme);
}

bool AtomURLRequestJobFactory::IsHandledURL(const GURL& url) const {
  if (!url.is_valid()) {
    // We handle error cases.
    return true;
  }
  return IsHandledProtocol(url.scheme());
}

}  // namespace atom
