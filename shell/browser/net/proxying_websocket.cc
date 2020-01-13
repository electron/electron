// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "electron/shell/browser/net/proxying_websocket.h"

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "components/keyed_service/content/browser_context_keyed_service_shutdown_notifier_factory.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_navigation_ui_data.h"
#include "net/base/ip_endpoint.h"
#include "net/http/http_util.h"

namespace electron {

ProxyingWebSocket::ProxyingWebSocket(
    scoped_refptr<api::RequestIDGenerator> request_id_generator,
    WebRequestAPI* web_request_api,
    WebSocketFactory factory,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client,
    bool has_extra_headers,
    int process_id,
    int render_frame_id,
    content::BrowserContext* browser_context)
    : web_request_api_(web_request_api),
      factory_(std::move(factory)),
      request_(request),
      forwarding_handshake_client_(std::move(handshake_client)),
      handshake_client_(std::move(handshake_client)),
      request_headers_(request.headers),
      has_extra_headers_(has_extra_headers) {
  info_.emplace(extensions::WebRequestInfoInitParams(
      request_id_generator->Generate(), process_id, render_frame_id, nullptr,
      MSG_ROUTING_NONE, request,
      /*is_download=*/false,
      /*is_async=*/true,
      /*is_service_worker_script=*/false));
}

ProxyingWebSocket::~ProxyingWebSocket() {
  // This is important to ensure that no outstanding blocking requests continue
  // to reference state owned by this object.

  if (on_before_send_headers_callback_) {
    std::move(on_before_send_headers_callback_)
        .Run(net::ERR_ABORTED, base::nullopt);
  }
  if (on_headers_received_callback_) {
    std::move(on_headers_received_callback_)
        .Run(net::ERR_ABORTED, base::nullopt, GURL());
  }
}

void ProxyingWebSocket::Start() {
  // If the header client will be used, we start the request immediately, and
  // OnBeforeSendHeaders and OnSendHeaders will be handled there. Otherwise,
  // send these events before the request starts.
  base::RepeatingCallback<void(int)> continuation;
  if (has_extra_headers_) {
    continuation = base::BindRepeating(
        &ProxyingWebSocket::ContinueToStartRequest, weak_factory_.GetWeakPtr());
  } else {
    continuation =
        base::BindRepeating(&ProxyingWebSocket::OnBeforeRequestComplete,
                            weak_factory_.GetWeakPtr());
  }

  // TODO(yhirano): Consider having throttling here (probably with aligned with
  // WebRequestProxyingURLLoaderFactory).
  bool should_collapse_initiator = false;
  int result = web_request_api_->OnBeforeRequest(&info_.value(), request_,
                                                 continuation, &redirect_url_);

  // It doesn't make sense to collapse WebSocket requests since they won't be
  // associated with a DOM element.
  DCHECK(!should_collapse_initiator);

  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    OnError(result);
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    return;
  }

  DCHECK_EQ(net::OK, result);
  continuation.Run(net::OK);
}

void ProxyingWebSocket::OnOpeningHandshakeStarted(
    network::mojom::WebSocketHandshakeRequestPtr request) {
  DCHECK(forwarding_handshake_client_);
  forwarding_handshake_client_->OnOpeningHandshakeStarted(std::move(request));
}

void ProxyingWebSocket::OnResponseReceived(
    network::mojom::WebSocketHandshakeResponsePtr response) {
  DCHECK(forwarding_handshake_client_);

  // response_.headers will be set in OnBeforeSendHeaders if
  // |receiver_as_header_client_| is set.
  if (!receiver_as_header_client_.is_bound()) {
    response_.headers =
        base::MakeRefCounted<net::HttpResponseHeaders>(base::StringPrintf(
            "HTTP/%d.%d %d %s", response->http_version.major_value(),
            response->http_version.minor_value(), response->status_code,
            response->status_text.c_str()));
    for (const auto& header : response->headers)
      response_.headers->AddHeader(header->name + ": " + header->value);
  }

  response_.remote_endpoint = response->remote_endpoint;

  // TODO(yhirano): OnResponseReceived is called with the original
  // response headers. That means if OnHeadersReceived modified them the
  // renderer won't see that modification. This is the opposite of http(s)
  // requests.
  forwarding_handshake_client_->OnResponseReceived(std::move(response));

  if (!receiver_as_header_client_.is_bound() || response_.headers) {
    ContinueToHeadersReceived();
  } else {
    waiting_for_header_client_headers_received_ = true;
  }
}

void ProxyingWebSocket::ContinueToHeadersReceived() {
  auto continuation =
      base::BindRepeating(&ProxyingWebSocket::OnHeadersReceivedComplete,
                          weak_factory_.GetWeakPtr());
  info_->AddResponseInfoFromResourceResponse(response_);
  int result = web_request_api_->OnHeadersReceived(
      &info_.value(), request_, continuation, response_.headers.get(),
      &override_headers_, &redirect_url_);

  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    OnError(result);
    return;
  }

  PauseIncomingMethodCallProcessing();
  if (result == net::ERR_IO_PENDING)
    return;

  DCHECK_EQ(net::OK, result);
  OnHeadersReceivedComplete(net::OK);
}

void ProxyingWebSocket::OnConnectionEstablished(
    mojo::PendingRemote<network::mojom::WebSocket> websocket,
    mojo::PendingReceiver<network::mojom::WebSocketClient> client_receiver,
    const std::string& selected_protocol,
    const std::string& extensions,
    mojo::ScopedDataPipeConsumerHandle readable) {
  DCHECK(forwarding_handshake_client_);
  DCHECK(!is_done_);
  is_done_ = true;
  web_request_api_->OnCompleted(&info_.value(), request_, net::ERR_WS_UPGRADE);

  forwarding_handshake_client_->OnConnectionEstablished(
      std::move(websocket), std::move(client_receiver), selected_protocol,
      extensions, std::move(readable));
}

void ProxyingWebSocket::OnAuthRequired(
    const net::AuthChallengeInfo& auth_info,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    const net::IPEndPoint& remote_endpoint,
    OnAuthRequiredCallback callback) {
  if (!callback) {
    OnError(net::ERR_FAILED);
    return;
  }

  response_.headers = headers;
  response_.remote_endpoint = remote_endpoint;
  auth_required_callback_ = std::move(callback);
}

void ProxyingWebSocket::OnBeforeSendHeaders(
    const net::HttpRequestHeaders& headers,
    OnBeforeSendHeadersCallback callback) {
  DCHECK(receiver_as_header_client_.is_bound());

  request_headers_ = headers;
  on_before_send_headers_callback_ = std::move(callback);
  OnBeforeRequestComplete(net::OK);
}

void ProxyingWebSocket::OnHeadersReceivedCompleteForAuth(
    const net::AuthChallengeInfo& auth_info,
    int rv) {
  if (rv != net::OK) {
    OnError(rv);
    return;
  }
  ResumeIncomingMethodCallProcessing();
  info_->AddResponseInfoFromResourceResponse(response_);

  auto continuation = base::BindRepeating(
      &ProxyingWebSocket::OnAuthRequiredComplete, weak_factory_.GetWeakPtr());
  // auto auth_rv = web_request_api_->OnAuthRequired(
  //     browser_context_, &info_, auth_info, std::move(continuation),
  //     &auth_credentials_);
  PauseIncomingMethodCallProcessing();
  // if (auth_rv == net::NetworkDelegate::AUTH_REQUIRED_RESPONSE_IO_PENDING)
  //   return;

  OnAuthRequiredComplete(net::NetworkDelegate::AUTH_REQUIRED_RESPONSE_SET_AUTH);
}

void ProxyingWebSocket::OnHeadersReceived(const std::string& headers,
                                          OnHeadersReceivedCallback callback) {
  DCHECK(receiver_as_header_client_.is_bound());

  // Note: since there are different pipes used for WebSocketClient and
  // TrustedHeaderClient, there are no guarantees whether this or
  // OnResponseReceived are called first.
  on_headers_received_callback_ = std::move(callback);
  response_.headers = base::MakeRefCounted<net::HttpResponseHeaders>(headers);

  if (!waiting_for_header_client_headers_received_)
    return;

  waiting_for_header_client_headers_received_ = false;
  ContinueToHeadersReceived();
}

void ProxyingWebSocket::StartProxying(
    WebRequestAPI* web_request_api,
    WebSocketFactory factory,
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<std::string>& user_agent,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client,
    bool has_extra_headers,
    int process_id,
    int render_frame_id,
    const url::Origin& origin,
    content::BrowserContext* browser_context,
    scoped_refptr<api::RequestIDGenerator> request_id_generator) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  network::ResourceRequest request;
  request.url = url;
  request.site_for_cookies = site_for_cookies;
  if (user_agent) {
    request.headers.SetHeader(net::HttpRequestHeaders::kUserAgent, *user_agent);
  }
  request.request_initiator = origin;
  auto* proxy = new ProxyingWebSocket(
      request_id_generator, web_request_api, std::move(factory), request,
      std::move(handshake_client), has_extra_headers, process_id,
      render_frame_id, browser_context);
  proxy->Start();
}

void ProxyingWebSocket::OnBeforeRequestComplete(int error_code) {
  DCHECK(receiver_as_header_client_.is_bound() ||
         !receiver_as_handshake_client_.is_bound());
  DCHECK(info_->url.SchemeIsWSOrWSS());
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  auto continuation =
      base::BindRepeating(&ProxyingWebSocket::OnBeforeSendHeadersComplete,
                          weak_factory_.GetWeakPtr());

  int result = web_request_api_->OnBeforeSendHeaders(
      &info_.value(), request_, continuation, &request_headers_);

  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    OnError(result);
    return;
  }

  if (result == net::ERR_IO_PENDING)
    return;

  DCHECK_EQ(net::OK, result);
  OnBeforeSendHeadersComplete(std::set<std::string>(), std::set<std::string>(),
                              net::OK);
}

void ProxyingWebSocket::OnBeforeSendHeadersComplete(
    const std::set<std::string>& removed_headers,
    const std::set<std::string>& set_headers,
    int error_code) {
  DCHECK(receiver_as_header_client_.is_bound() ||
         !receiver_as_handshake_client_.is_bound());
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  if (receiver_as_header_client_.is_bound()) {
    DCHECK(on_before_send_headers_callback_);
    std::move(on_before_send_headers_callback_)
        .Run(error_code, request_headers_);
  }

  web_request_api_->OnSendHeaders(&info_.value(), request_, request_headers_);

  if (!receiver_as_header_client_.is_bound())
    ContinueToStartRequest(net::OK);
}

void ProxyingWebSocket::ContinueToStartRequest(int error_code) {
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }
  base::flat_set<std::string> used_header_names;
  std::vector<network::mojom::HttpHeaderPtr> additional_headers;
  for (net::HttpRequestHeaders::Iterator it(request_headers_); it.GetNext();) {
    additional_headers.push_back(
        network::mojom::HttpHeader::New(it.name(), it.value()));
    used_header_names.insert(base::ToLowerASCII(it.name()));
  }
  for (const auto& header : additional_headers_) {
    if (!used_header_names.contains(base::ToLowerASCII(header->name))) {
      additional_headers.push_back(
          network::mojom::HttpHeader::New(header->name, header->value));
    }
  }

  mojo::PendingRemote<network::mojom::TrustedHeaderClient>
      trusted_header_client = mojo::NullRemote();
  if (has_extra_headers_) {
    trusted_header_client =
        receiver_as_header_client_.BindNewPipeAndPassRemote();
  }

  std::move(factory_).Run(
      info_->url, std::move(additional_headers),
      receiver_as_handshake_client_.BindNewPipeAndPassRemote(),
      receiver_as_auth_handler_.BindNewPipeAndPassRemote(),
      std::move(trusted_header_client));

  // Here we detect mojo connection errors on |receiver_as_handshake_client_|.
  // See also CreateWebSocket in
  // //network/services/public/mojom/network_context.mojom.
  receiver_as_handshake_client_.set_disconnect_with_reason_handler(
      base::BindOnce(&ProxyingWebSocket::OnMojoConnectionError,
                     base::Unretained(this)));
}

void ProxyingWebSocket::OnHeadersReceivedComplete(int error_code) {
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  if (on_headers_received_callback_) {
    base::Optional<std::string> headers;
    if (override_headers_)
      headers = override_headers_->raw_headers();
    std::move(on_headers_received_callback_).Run(net::OK, headers, GURL());
  }

  if (override_headers_) {
    response_.headers = override_headers_;
    override_headers_ = nullptr;
  }

  ResumeIncomingMethodCallProcessing();
  info_->AddResponseInfoFromResourceResponse(response_);
  web_request_api_->OnResponseStarted(&info_.value(), request_);
}

void ProxyingWebSocket::OnAuthRequiredComplete(
    net::NetworkDelegate::AuthRequiredResponse rv) {
  DCHECK(auth_required_callback_);
  ResumeIncomingMethodCallProcessing();
  switch (rv) {
    case net::NetworkDelegate::AUTH_REQUIRED_RESPONSE_NO_ACTION:
    case net::NetworkDelegate::AUTH_REQUIRED_RESPONSE_CANCEL_AUTH:
      std::move(auth_required_callback_).Run(base::nullopt);
      break;

    case net::NetworkDelegate::AUTH_REQUIRED_RESPONSE_SET_AUTH:
      std::move(auth_required_callback_).Run(auth_credentials_);
      break;
    case net::NetworkDelegate::AUTH_REQUIRED_RESPONSE_IO_PENDING:
      NOTREACHED();
      break;
  }
}

void ProxyingWebSocket::PauseIncomingMethodCallProcessing() {
  receiver_as_handshake_client_.Pause();
  receiver_as_auth_handler_.Pause();
  if (receiver_as_header_client_.is_bound())
    receiver_as_header_client_.Pause();
}

void ProxyingWebSocket::ResumeIncomingMethodCallProcessing() {
  receiver_as_handshake_client_.Resume();
  receiver_as_auth_handler_.Resume();
  if (receiver_as_header_client_.is_bound())
    receiver_as_header_client_.Resume();
}

void ProxyingWebSocket::OnError(int error_code) {
  if (!is_done_) {
    is_done_ = true;
    web_request_api_->OnErrorOccurred(&info_.value(), request_, error_code);
  }
}

void ProxyingWebSocket::OnMojoConnectionError(uint32_t custom_reason,
                                              const std::string& description) {
  forwarding_handshake_client_.ResetWithReason(custom_reason, description);
  OnError(net::ERR_FAILED);
  // Deletes |this|.
}

}  // namespace electron
