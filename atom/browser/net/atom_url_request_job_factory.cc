// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_url_request_job_factory.h"

#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;

namespace atom {

namespace {

int disable_protocol_intercept_flag_key = 0;

}  // namespace

typedef net::URLRequestJobFactory::ProtocolHandler ProtocolHandler;

const void* DisableProtocolInterceptFlagKey() {
  return &disable_protocol_intercept_flag_key;
}

AtomURLRequestJobFactory::AtomURLRequestJobFactory() {}

AtomURLRequestJobFactory::~AtomURLRequestJobFactory() {
  Clear();
}

bool AtomURLRequestJobFactory::SetProtocolHandler(
    const std::string& scheme,
    std::unique_ptr<ProtocolHandler> protocol_handler) {
  if (!protocol_handler) {
    auto it = protocol_handler_map_.find(scheme);
    if (it == protocol_handler_map_.end())
      return false;

    delete it->second;
    protocol_handler_map_.erase(it);
    return true;
  }

  if (base::ContainsKey(protocol_handler_map_, scheme))
    return false;
  protocol_handler_map_[scheme] = protocol_handler.release();
  return true;
}

bool AtomURLRequestJobFactory::InterceptProtocol(
    const std::string& scheme,
    std::unique_ptr<ProtocolHandler> protocol_handler) {
  if (!base::ContainsKey(protocol_handler_map_, scheme) ||
      base::ContainsKey(original_protocols_, scheme))
    return false;
  ProtocolHandler* original_protocol_handler = protocol_handler_map_[scheme];
  protocol_handler_map_[scheme] = protocol_handler.release();
  original_protocols_[scheme].reset(original_protocol_handler);
  return true;
}

bool AtomURLRequestJobFactory::UninterceptProtocol(const std::string& scheme) {
  auto it = original_protocols_.find(scheme);
  if (it == original_protocols_.end())
    return false;
  protocol_handler_map_[scheme] = it->second.release();
  original_protocols_.erase(it);
  return true;
}

ProtocolHandler* AtomURLRequestJobFactory::GetProtocolHandler(
    const std::string& scheme) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  auto it = protocol_handler_map_.find(scheme);
  if (it == protocol_handler_map_.end())
    return nullptr;
  return it->second;
}

bool AtomURLRequestJobFactory::HasProtocolHandler(
    const std::string& scheme) const {
  return base::ContainsKey(protocol_handler_map_, scheme);
}

void AtomURLRequestJobFactory::Clear() {
  for (auto& it : protocol_handler_map_)
    delete it.second;
  protocol_handler_map_.clear();
  original_protocols_.clear();
}

net::URLRequestJob* AtomURLRequestJobFactory::MaybeCreateJobWithProtocolHandler(
    const std::string& scheme,
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  auto it = protocol_handler_map_.find(scheme);
  if (it == protocol_handler_map_.end())
    return nullptr;
  if (request->GetUserData(DisableProtocolInterceptFlagKey()))
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

bool AtomURLRequestJobFactory::IsSafeRedirectTarget(
    const GURL& location) const {
  if (!location.is_valid()) {
    // We handle error cases.
    return true;
  }
  return IsHandledProtocol(location.scheme());
}

}  // namespace atom
