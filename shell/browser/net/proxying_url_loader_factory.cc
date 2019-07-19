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
    int32_t routing_id,
    int32_t network_service_request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    network::mojom::URLLoaderRequest loader_request,
    network::mojom::URLLoaderClientPtr client)
    : factory_(factory),
      request_(request),
      original_initiator_(request.request_initiator),
      routing_id_(routing_id),
      network_service_request_id_(network_service_request_id),
      options_(options),
      traffic_annotation_(traffic_annotation),
      proxied_loader_binding_(this, std::move(loader_request)),
      target_client_(std::move(client)),
      proxied_client_binding_(this),
      // TODO(zcbenz): We should always use "extraHeaders" mode to be compatible
      // with old APIs.
      has_any_extra_headers_listeners_(false),
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
      network_service_request_id_ != 0 &&
      false /* HasExtraHeadersListenerForRequest */;
}

void ProxyingURLLoaderFactory::InProgressRequest::RestartInternal() {
  request_completed_ = false;

  // If the header client will be used, we start the request immediately, and
  // OnBeforeSendHeaders and OnSendHeaders will be handled there. Otherwise,
  // send these events before the request starts.
  base::RepeatingCallback<void(int)> continuation;
  if (current_request_uses_header_client_) {
    continuation = base::BindRepeating(
        &InProgressRequest::ContinueToStartRequest, weak_factory_.GetWeakPtr());
  } else {
    continuation =
        base::BindRepeating(&InProgressRequest::ContinueToBeforeSendHeaders,
                            weak_factory_.GetWeakPtr());
  }
  // TODO(zcbenz): Call webRequest.onBeforeRequest.
  int result = net::OK;
  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    // The request was cancelled synchronously. Dispatch an error notification
    // and terminate the request.
    network::URLLoaderCompletionStatus status(result);
    OnRequestError(status);
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    // One or more listeners is blocking, so the request must be paused until
    // they respond. |continuation| above will be invoked asynchronously to
    // continue or cancel the request.
    //
    // We pause the binding here to prevent further client message processing.
    if (proxied_client_binding_.is_bound())
      proxied_client_binding_.PauseIncomingMethodCallProcessing();

    // Pause the header client, since we want to wait until OnBeforeRequest has
    // finished before processing any future events.
    if (header_client_binding_)
      header_client_binding_.PauseIncomingMethodCallProcessing();
    return;
  }
  DCHECK_EQ(net::OK, result);

  continuation.Run(net::OK);
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

  request_.headers = headers;
  on_before_send_headers_callback_ = std::move(callback);
  ContinueToBeforeSendHeaders(net::OK);
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

void ProxyingURLLoaderFactory::InProgressRequest::ContinueToBeforeSendHeaders(
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (proxied_client_binding_.is_bound())
    proxied_client_binding_.ResumeIncomingMethodCallProcessing();

  auto continuation = base::BindRepeating(
      &InProgressRequest::ContinueToSendHeaders, weak_factory_.GetWeakPtr());
  // Note: In Electron onBeforeSendHeaders is called for all protocols.
  // TODO(zcbenz): Call webRequest.onBeforeSendHeaders.
  int result = net::OK;

  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    // The request was cancelled synchronously. Dispatch an error notification
    // and terminate the request.
    OnRequestError(network::URLLoaderCompletionStatus(result));
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    // One or more listeners is blocking, so the request must be paused until
    // they respond. |continuation| above will be invoked asynchronously to
    // continue or cancel the request.
    //
    // We pause the binding here to prevent further client message processing.
    if (proxied_client_binding_.is_bound())
      proxied_client_binding_.PauseIncomingMethodCallProcessing();
    return;
  }
  DCHECK_EQ(net::OK, result);

  ContinueToSendHeaders(std::set<std::string>(), std::set<std::string>(),
                        net::OK);
}

void ProxyingURLLoaderFactory::InProgressRequest::ContinueToSendHeaders(
    const std::set<std::string>& removed_headers,
    const std::set<std::string>& set_headers,
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (current_request_uses_header_client_) {
    DCHECK(on_before_send_headers_callback_);
    std::move(on_before_send_headers_callback_)
        .Run(error_code, request_.headers);
  } else if (pending_follow_redirect_params_) {
    pending_follow_redirect_params_->removed_headers.insert(
        pending_follow_redirect_params_->removed_headers.end(),
        removed_headers.begin(), removed_headers.end());

    for (auto& set_header : set_headers) {
      std::string header_value;
      if (request_.headers.GetHeader(set_header, &header_value)) {
        pending_follow_redirect_params_->modified_headers.SetHeader(
            set_header, header_value);
      } else {
        NOTREACHED();
      }
    }

    if (target_loader_.is_bound()) {
      target_loader_->FollowRedirect(
          pending_follow_redirect_params_->removed_headers,
          pending_follow_redirect_params_->modified_headers,
          pending_follow_redirect_params_->new_url);
    }

    pending_follow_redirect_params_.reset();
  }

  if (proxied_client_binding_.is_bound())
    proxied_client_binding_.ResumeIncomingMethodCallProcessing();

  // Note: In Electron onSendHeaders is called for all protocols.
  // TODO(zcbenz): Call webRequest.onSendHeaders.

  if (!current_request_uses_header_client_)
    ContinueToStartRequest(net::OK);
}

void ProxyingURLLoaderFactory::InProgressRequest::ContinueToStartRequest(
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (proxied_client_binding_.is_bound())
    proxied_client_binding_.ResumeIncomingMethodCallProcessing();

  if (header_client_binding_)
    header_client_binding_.ResumeIncomingMethodCallProcessing();

  if (!target_loader_.is_bound() && factory_->target_factory_.is_bound()) {
    // No extensions have cancelled us up to this point, so it's now OK to
    // initiate the real network request.
    network::mojom::URLLoaderClientPtr proxied_client;
    proxied_client_binding_.Bind(mojo::MakeRequest(&proxied_client));
    uint32_t options = options_;
    // Even if this request does not use the header client, future redirects
    // might, so we need to set the option on the loader.
    if (has_any_extra_headers_listeners_)
      options |= network::mojom::kURLLoadOptionUseHeaderClient;
    factory_->target_factory_->CreateLoaderAndStart(
        mojo::MakeRequest(&target_loader_), routing_id_,
        network_service_request_id_, options, request_,
        std::move(proxied_client), traffic_annotation_);
  }

  // From here the lifecycle of this request is driven by subsequent events on
  // either |proxy_loader_binding_|, |proxy_client_binding_|, or
  // |header_client_binding_|.
}

void ProxyingURLLoaderFactory::InProgressRequest::OnRequestError(
    const network::URLLoaderCompletionStatus& status) {
  if (!request_completed_) {
    target_client_->OnComplete(status);
    // TODO(zcbenz): Call webRequest.onError.
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
