// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/proxying_url_loader_factory.h"

#include <utility>

#include "mojo/public/cpp/bindings/binding.h"
#include "shell/browser/net/asar/asar_url_loader.h"

namespace electron {

ProxyingURLLoaderFactory::InProgressRequest::FollowRedirectParams::
    FollowRedirectParams() = default;
ProxyingURLLoaderFactory::InProgressRequest::FollowRedirectParams::
    ~FollowRedirectParams() = default;

ProxyingURLLoaderFactory::InProgressRequest::InProgressRequest(
    ProxyingURLLoaderFactory* factory,
    // int32_t routing_id,
    int32_t network_service_request_id,
    // uint32_t options,
    const network::ResourceRequest& request,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    network::mojom::URLLoaderRequest loader_request,
    network::mojom::URLLoaderClientPtr client)
    : factory_(factory),
      request_(request),
      original_initiator_(request.request_initiator),
      // routing_id_(routing_id),
      network_service_request_id_(network_service_request_id),
      // options_(options),
      traffic_annotation_(traffic_annotation),
      proxied_loader_binding_(this, std::move(loader_request)),
      target_client_(std::move(client)),
      proxied_client_binding_(this),
      // TODO(zcbenz): We should always use "extraHeaders" mode to be compatible
      // with old APIs.
      // has_any_extra_headers_listeners_(false),
      header_client_binding_(this) {
  // If there is a client error, clean up the request.
  target_client_.set_connection_error_handler(base::BindOnce(
      &ProxyingURLLoaderFactory::InProgressRequest::OnRequestError,
      weak_factory_.GetWeakPtr(),
      network::URLLoaderCompletionStatus(net::ERR_ABORTED)));
}

ProxyingURLLoaderFactory::InProgressRequest::~InProgressRequest() {}

void ProxyingURLLoaderFactory::InProgressRequest::Restart() {
  UpdateRequestInfo();
  RestartInternal();
}

void ProxyingURLLoaderFactory::InProgressRequest::UpdateRequestInfo() {
  current_request_uses_header_client_ =
      factory_->url_loader_header_client_binding_ &&
      request_.url.SchemeIsHTTPOrHTTPS() && network_service_request_id_ != 0 &&
      false /* HasExtraHeadersListenerForRequest */;
}

void ProxyingURLLoaderFactory::InProgressRequest::RestartInternal() {
  // TODO(zcbenz): Implement me.
}

void ProxyingURLLoaderFactory::InProgressRequest::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const base::Optional<GURL>& new_url) {
  if (new_url)
    request_.url = new_url.value();

  for (const std::string& header : removed_headers)
    request_.headers.RemoveHeader(header);
  request_.headers.MergeFrom(modified_headers);

  // Call this before checking |current_request_uses_header_client_| as it
  // calculates it.
  UpdateRequestInfo();

  if (target_loader_.is_bound()) {
    // If header_client_ is used, then we have to call FollowRedirect now as
    // that's what triggers the network service calling back to
    // OnBeforeSendHeaders(). Otherwise, don't call FollowRedirect now. Wait for
    // the onBeforeSendHeaders callback(s) to run as these may modify request
    // headers and if so we'll pass these modifications to FollowRedirect.
    if (current_request_uses_header_client_) {
      target_loader_->FollowRedirect(removed_headers, modified_headers,
                                     new_url);
    } else {
      auto params = std::make_unique<FollowRedirectParams>();
      params->removed_headers = removed_headers;
      params->modified_headers = modified_headers;
      params->new_url = new_url;
      pending_follow_redirect_params_ = std::move(params);
    }
  }

  RestartInternal();
}

void ProxyingURLLoaderFactory::InProgressRequest::ProceedWithResponse() {
  if (target_loader_.is_bound())
    target_loader_->ProceedWithResponse();
}

void ProxyingURLLoaderFactory::InProgressRequest::SetPriority(
    net::RequestPriority priority,
    int32_t intra_priority_value) {
  if (target_loader_.is_bound())
    target_loader_->SetPriority(priority, intra_priority_value);
}

void ProxyingURLLoaderFactory::InProgressRequest::PauseReadingBodyFromNet() {
  if (target_loader_.is_bound())
    target_loader_->PauseReadingBodyFromNet();
}

void ProxyingURLLoaderFactory::InProgressRequest::ResumeReadingBodyFromNet() {
  if (target_loader_.is_bound())
    target_loader_->ResumeReadingBodyFromNet();
}

void ProxyingURLLoaderFactory::InProgressRequest::OnReceiveResponse(
    const network::ResourceResponseHead& head) {
  // TODO(zcbenz): Implement me.
}

void ProxyingURLLoaderFactory::InProgressRequest::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseHead& head) {
  // TODO(zcbenz): Implement me.
}

void ProxyingURLLoaderFactory::InProgressRequest::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback callback) {
  target_client_->OnUploadProgress(current_position, total_size,
                                   std::move(callback));
}

void ProxyingURLLoaderFactory::InProgressRequest::OnReceiveCachedMetadata(
    mojo_base::BigBuffer data) {
  target_client_->OnReceiveCachedMetadata(std::move(data));
}

void ProxyingURLLoaderFactory::InProgressRequest::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  target_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void ProxyingURLLoaderFactory::InProgressRequest::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  target_client_->OnStartLoadingResponseBody(std::move(body));
}

void ProxyingURLLoaderFactory::InProgressRequest::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  // TODO(zcbenz): Implement me.
}

void ProxyingURLLoaderFactory::InProgressRequest::OnLoaderCreated(
    network::mojom::TrustedHeaderClientRequest request) {
  header_client_binding_.Bind(std::move(request));
}

void ProxyingURLLoaderFactory::InProgressRequest::OnBeforeSendHeaders(
    const net::HttpRequestHeaders& headers,
    OnBeforeSendHeadersCallback callback) {
  if (!current_request_uses_header_client_) {
    std::move(callback).Run(net::OK, base::nullopt);
    return;
  }

  // TODO(zcbenz): Implement me.
}

void ProxyingURLLoaderFactory::InProgressRequest::OnHeadersReceived(
    const std::string& headers,
    OnHeadersReceivedCallback callback) {
  if (!current_request_uses_header_client_) {
    std::move(callback).Run(net::OK, base::nullopt, GURL());
    return;
  }

  // TODO(zcbenz): Implement me.
}

void ProxyingURLLoaderFactory::InProgressRequest::OnRequestError(
    const network::URLLoaderCompletionStatus& status) {
  if (!request_completed_) {
    target_client_->OnComplete(status);
    // TODO(zcbenz): Report error.
  }

  // TODO(zcbenz): Deletes |this|.
}

ProxyingURLLoaderFactory::ProxyingURLLoaderFactory(
    const HandlersMap& intercepted_handlers,
    network::mojom::URLLoaderFactoryRequest loader_request,
    network::mojom::URLLoaderFactoryPtrInfo target_factory_info,
    network::mojom::TrustedURLLoaderHeaderClientRequest header_client_request)
    : intercepted_handlers_(intercepted_handlers),
      url_loader_header_client_binding_(this) {
  target_factory_.Bind(std::move(target_factory_info));
  target_factory_.set_connection_error_handler(base::BindOnce(
      &ProxyingURLLoaderFactory::OnTargetFactoryError, base::Unretained(this)));
  proxy_bindings_.AddBinding(this, std::move(loader_request));
  proxy_bindings_.set_connection_error_handler(base::BindRepeating(
      &ProxyingURLLoaderFactory::OnProxyBindingError, base::Unretained(this)));

  if (header_client_request)
    url_loader_header_client_binding_.Bind(std::move(header_client_request));
}

ProxyingURLLoaderFactory::~ProxyingURLLoaderFactory() = default;

void ProxyingURLLoaderFactory::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  // Check if user has intercepted this scheme.
  auto it = intercepted_handlers_.find(request.url.scheme());
  if (it != intercepted_handlers_.end()) {
    // <scheme, <type, handler>>
    it->second.second.Run(
        request, base::BindOnce(&AtomURLLoaderFactory::StartLoading,
                                std::move(loader), routing_id, request_id,
                                options, request, std::move(client),
                                traffic_annotation, this, it->second.first));
    return;
  }

  // Intercept file:// protocol to support asar archives.
  if (request.url.SchemeIsFile()) {
    asar::CreateAsarURLLoader(request, std::move(loader), std::move(client),
                              nullptr);
    return;
  }

  // Pass-through to the original factory.
  target_factory_->CreateLoaderAndStart(std::move(loader), routing_id,
                                        request_id, options, request,
                                        std::move(client), traffic_annotation);
}

void ProxyingURLLoaderFactory::Clone(
    network::mojom::URLLoaderFactoryRequest loader_request) {
  proxy_bindings_.AddBinding(this, std::move(loader_request));
}

void ProxyingURLLoaderFactory::OnLoaderCreated(
    int32_t request_id,
    network::mojom::TrustedHeaderClientRequest request) {
  // TODO(zcbenz): Call |OnLoaderCreated| for |InProgressRequest|.
}

void ProxyingURLLoaderFactory::OnTargetFactoryError() {
  delete this;
}

void ProxyingURLLoaderFactory::OnProxyBindingError() {
  if (proxy_bindings_.empty())
    delete this;
}

}  // namespace electron
