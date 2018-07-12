// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/resolve_proxy_helper.h"

#include "base/threading/thread_task_runner_handle.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace atom {

ResolveProxyHelper::ResolveProxyHelper(net::URLRequestContextGetter* getter)
    : context_getter_(getter),
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

void ResolveProxyHelper::SendProxyResult(const std::string& proxy) {
  CHECK(!pending_requests_.empty());

  const auto& completed_request = pending_requests_.front();
  if (!completed_request.callback.is_null())
    completed_request.callback.Run(proxy);

  // Clear the current (completed) request.
  pending_requests_.pop_front();

  // Start the next request.
  if (!pending_requests_.empty())
    StartPendingRequest();
}

void ResolveProxyHelper::StartPendingRequest() {
  auto& request = pending_requests_.front();
  context_getter_->GetNetworkTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&ResolveProxyHelper::StartPendingRequestInIO,
                     base::Unretained(this), request.url, request.pac_req));
}

void ResolveProxyHelper::OnResolveProxyCompleted(int result) {
  std::string proxy;
  if (result == net::OK)
    proxy = proxy_info_.ToPacString();

  original_thread_->PostTask(
      FROM_HERE, base::BindOnce(&ResolveProxyHelper::SendProxyResult,
                                base::Unretained(this), proxy));
}

void ResolveProxyHelper::StartPendingRequestInIO(
    const GURL& url,
    net::ProxyService::PacRequest* pac_req) {
  // Verify the request wasn't started yet.
  DCHECK(nullptr == pac_req);

  auto proxy_service = context_getter_->GetURLRequestContext()->proxy_service();

  // Start the request.
  int result = proxy_service->ResolveProxy(
      url, std::string(), &proxy_info_,
      base::Bind(&ResolveProxyHelper::OnResolveProxyCompleted,
                 base::Unretained(this)),
      &pac_req, nullptr, net::NetLogWithSource());

  // Completed synchronously.
  if (result != net::ERR_IO_PENDING)
    OnResolveProxyCompleted(result);
}

}  // namespace atom
