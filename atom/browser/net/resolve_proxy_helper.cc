// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/resolve_proxy_helper.h"

#include "atom/browser/atom_browser_context.h"
#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace atom {

ResolveProxyHelper::ResolveProxyHelper(AtomBrowserContext* browser_context)
    : context_getter_(browser_context->GetRequestContext()),
      original_thread_(base::ThreadTaskRunnerHandle::Get()) {}

ResolveProxyHelper::~ResolveProxyHelper() {
  // Clear all pending requests if the ProxyService is still alive.
  pending_requests_.clear();
}

void ResolveProxyHelper::ResolveProxy(const GURL& url,
                                      const ResolveProxyCallback& callback) {
  // Enqueue the pending request.
  pending_requests_.push_back(PendingRequest(url, callback));

  // If nothing is in progress, start.
  if (pending_requests_.size() == 1)
    StartPendingRequest();
}

void ResolveProxyHelper::StartPendingRequest() {
  auto& pending_request = pending_requests_.front();
  context_getter_->GetNetworkTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&ResolveProxyHelper::StartPendingRequestInIO,
                                base::Unretained(this), pending_request.url));
}

void ResolveProxyHelper::StartPendingRequestInIO(const GURL& url) {
  auto* proxy_service =
      context_getter_->GetURLRequestContext()->proxy_resolution_service();
  // Start the request.
  int result = proxy_service->ResolveProxy(
      url, std::string(), &proxy_info_,
      base::Bind(&ResolveProxyHelper::OnProxyResolveComplete,
                 base::RetainedRef(this)),
      nullptr, nullptr, net::NetLogWithSource());
  // Completed synchronously.
  if (result != net::ERR_IO_PENDING)
    OnProxyResolveComplete(result);
}

void ResolveProxyHelper::OnProxyResolveComplete(int result) {
  DCHECK(!pending_requests_.empty());

  std::string proxy;
  if (result == net::OK)
    proxy = proxy_info_.ToPacString();

  original_thread_->PostTask(
      FROM_HERE, base::BindOnce(&ResolveProxyHelper::SendProxyResult,
                                base::RetainedRef(this), proxy));
}

void ResolveProxyHelper::SendProxyResult(const std::string& proxy) {
  DCHECK(!pending_requests_.empty());

  const auto& completed_request = pending_requests_.front();
  if (!completed_request.callback.is_null())
    completed_request.callback.Run(proxy);

  // Clear the current (completed) request.
  pending_requests_.pop_front();

  // Start the next request.
  if (!pending_requests_.empty())
    StartPendingRequest();
}

ResolveProxyHelper::PendingRequest::PendingRequest(
    const GURL& url,
    const ResolveProxyCallback& callback)
    : url(url), callback(callback) {}

}  // namespace atom
