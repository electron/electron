// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/url_loader_header_client_chain.h"

#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/ssl/ssl_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "url/gurl.h"

namespace electron {

namespace {

using network::mojom::TrustedHeaderClient;
using network::mojom::TrustedURLLoaderHeaderClient;

// Relays the header events of one network::URLLoader to both proxies.
//
// A proxy that closes its end has no further say over the loader: it is
// skipped, and a reply it still owed counts as "no change". A proxy drops its
// end when it does not track the request or when its request is gone, and in
// the latter case it has also closed the URLLoader, which ends the request.
class HeaderClientChain : public TrustedHeaderClient {
 public:
  HeaderClientChain(mojo::PendingReceiver<TrustedHeaderClient> receiver,
                    mojo::PendingRemote<TrustedHeaderClient> outer,
                    mojo::PendingRemote<TrustedHeaderClient> inner)
      : receiver_(this, std::move(receiver)),
        outer_(std::move(outer)),
        inner_(std::move(inner)) {
    receiver_.set_disconnect_handler(
        base::BindOnce(&HeaderClientChain::DeleteThis, base::Unretained(this)));
    outer_.reset_on_disconnect();
    inner_.reset_on_disconnect();
  }

  HeaderClientChain(const HeaderClientChain&) = delete;
  HeaderClientChain& operator=(const HeaderClientChain&) = delete;

 private:
  ~HeaderClientChain() override = default;

  void DeleteThis() { delete this; }

  // network::mojom::TrustedHeaderClient:
  void OnBeforeSendHeaders(const GURL& request_url,
                           const net::HttpRequestHeaders& headers,
                           OnBeforeSendHeadersCallback callback) override {
    if (!outer_) {
      OnOuterBeforeSendHeaders(request_url, headers, std::move(callback),
                               net::OK, std::nullopt, std::nullopt);
      return;
    }
    outer_->OnBeforeSendHeaders(
        request_url, headers,
        mojo::WrapCallbackWithDefaultInvokeIfNotRun(
            base::BindOnce(&HeaderClientChain::OnOuterBeforeSendHeaders,
                           weak_factory_.GetWeakPtr(), request_url, headers,
                           std::move(callback)),
            net::OK, std::optional<net::HttpRequestHeaders>(),
            std::optional<base::DictValue>()));
  }

  void OnHeadersReceived(const std::string& headers,
                         const net::IPEndPoint& remote_endpoint,
                         const std::optional<net::SSLInfo>& ssl_info,
                         OnHeadersReceivedCallback callback) override {
    if (!inner_) {
      OnInnerHeadersReceived(headers, remote_endpoint, ssl_info,
                             std::move(callback), net::OK, std::nullopt,
                             std::nullopt);
      return;
    }
    inner_->OnHeadersReceived(
        headers, remote_endpoint, ssl_info,
        mojo::WrapCallbackWithDefaultInvokeIfNotRun(
            base::BindOnce(&HeaderClientChain::OnInnerHeadersReceived,
                           weak_factory_.GetWeakPtr(), headers, remote_endpoint,
                           ssl_info, std::move(callback)),
            net::OK, std::optional<std::string>(), std::optional<GURL>()));
  }

  void OnOuterBeforeSendHeaders(
      const GURL& request_url,
      const net::HttpRequestHeaders& original_headers,
      OnBeforeSendHeadersCallback callback,
      int32_t result,
      const std::optional<net::HttpRequestHeaders>& headers,
      std::optional<base::DictValue> net_log_events) {
    if (result != net::OK || !inner_) {
      std::move(callback).Run(result, headers, std::move(net_log_events));
      return;
    }
    inner_->OnBeforeSendHeaders(
        request_url, headers ? *headers : original_headers,
        mojo::WrapCallbackWithDefaultInvokeIfNotRun(
            base::BindOnce(&HeaderClientChain::OnInnerBeforeSendHeaders,
                           headers, std::move(net_log_events),
                           std::move(callback)),
            net::OK, std::optional<net::HttpRequestHeaders>(),
            std::optional<base::DictValue>()));
  }

  static void OnInnerBeforeSendHeaders(
      const std::optional<net::HttpRequestHeaders>& outer_headers,
      std::optional<base::DictValue> outer_net_log_events,
      OnBeforeSendHeadersCallback callback,
      int32_t result,
      const std::optional<net::HttpRequestHeaders>& headers,
      std::optional<base::DictValue> net_log_events) {
    std::move(callback).Run(result, headers ? headers : outer_headers,
                            net_log_events ? std::move(net_log_events)
                                           : std::move(outer_net_log_events));
  }

  void OnInnerHeadersReceived(
      const std::string& original_headers,
      const net::IPEndPoint& remote_endpoint,
      const std::optional<net::SSLInfo>& ssl_info,
      OnHeadersReceivedCallback callback,
      int32_t result,
      const std::optional<std::string>& headers,
      const std::optional<GURL>& preserve_fragment_on_redirect_url) {
    if (result != net::OK || !outer_) {
      std::move(callback).Run(result, headers,
                              preserve_fragment_on_redirect_url);
      return;
    }
    outer_->OnHeadersReceived(
        headers ? *headers : original_headers, remote_endpoint, ssl_info,
        mojo::WrapCallbackWithDefaultInvokeIfNotRun(
            base::BindOnce(&HeaderClientChain::OnOuterHeadersReceived, headers,
                           preserve_fragment_on_redirect_url,
                           std::move(callback)),
            net::OK, std::optional<std::string>(), std::optional<GURL>()));
  }

  static void OnOuterHeadersReceived(
      const std::optional<std::string>& inner_headers,
      const std::optional<GURL>& inner_preserve_fragment_on_redirect_url,
      OnHeadersReceivedCallback callback,
      int32_t result,
      const std::optional<std::string>& headers,
      const std::optional<GURL>& preserve_fragment_on_redirect_url) {
    const bool has_url = preserve_fragment_on_redirect_url &&
                         !preserve_fragment_on_redirect_url->is_empty();
    std::move(callback).Run(result, headers ? headers : inner_headers,
                            has_url ? preserve_fragment_on_redirect_url
                                    : inner_preserve_fragment_on_redirect_url);
  }

  mojo::Receiver<TrustedHeaderClient> receiver_;
  mojo::Remote<TrustedHeaderClient> outer_;
  mojo::Remote<TrustedHeaderClient> inner_;
  base::WeakPtrFactory<HeaderClientChain> weak_factory_{this};
};

class URLLoaderHeaderClientChain : public TrustedURLLoaderHeaderClient {
 public:
  URLLoaderHeaderClientChain(
      mojo::PendingReceiver<TrustedURLLoaderHeaderClient> receiver,
      mojo::PendingRemote<TrustedURLLoaderHeaderClient> outer,
      mojo::PendingRemote<TrustedURLLoaderHeaderClient> inner,
      HandlesLoaderCallback inner_handles_loader)
      : receiver_(this, std::move(receiver)),
        outer_(std::move(outer)),
        inner_(std::move(inner)),
        inner_handles_loader_(std::move(inner_handles_loader)) {
    receiver_.set_disconnect_handler(base::BindOnce(
        &URLLoaderHeaderClientChain::DeleteThis, base::Unretained(this)));
  }

  URLLoaderHeaderClientChain(const URLLoaderHeaderClientChain&) = delete;
  URLLoaderHeaderClientChain& operator=(const URLLoaderHeaderClientChain&) =
      delete;

 private:
  ~URLLoaderHeaderClientChain() override = default;

  void DeleteThis() { delete this; }

  // network::mojom::TrustedURLLoaderHeaderClient:
  void OnLoaderCreated(
      int32_t request_id,
      mojo::PendingReceiver<TrustedHeaderClient> receiver) override {
    if (!inner_handles_loader_.Run(request_id)) {
      outer_->OnLoaderCreated(request_id, std::move(receiver));
      return;
    }
    mojo::PendingRemote<TrustedHeaderClient> outer;
    mojo::PendingRemote<TrustedHeaderClient> inner;
    outer_->OnLoaderCreated(request_id, outer.InitWithNewPipeAndPassReceiver());
    inner_->OnLoaderCreated(request_id, inner.InitWithNewPipeAndPassReceiver());
    new HeaderClientChain(std::move(receiver), std::move(outer),
                          std::move(inner));
  }

  void OnLoaderForCorsPreflightCreated(
      const network::ResourceRequest& request,
      mojo::PendingReceiver<TrustedHeaderClient> receiver) override {
    if (!inner_handles_loader_.Run(std::nullopt)) {
      outer_->OnLoaderForCorsPreflightCreated(request, std::move(receiver));
      return;
    }
    mojo::PendingRemote<TrustedHeaderClient> outer;
    mojo::PendingRemote<TrustedHeaderClient> inner;
    outer_->OnLoaderForCorsPreflightCreated(
        request, outer.InitWithNewPipeAndPassReceiver());
    inner_->OnLoaderForCorsPreflightCreated(
        request, inner.InitWithNewPipeAndPassReceiver());
    new HeaderClientChain(std::move(receiver), std::move(outer),
                          std::move(inner));
  }

  mojo::Receiver<TrustedURLLoaderHeaderClient> receiver_;
  mojo::Remote<TrustedURLLoaderHeaderClient> outer_;
  mojo::Remote<TrustedURLLoaderHeaderClient> inner_;
  const HandlesLoaderCallback inner_handles_loader_;
};

}  // namespace

void ChainURLLoaderHeaderClients(
    mojo::PendingReceiver<TrustedURLLoaderHeaderClient> receiver,
    mojo::PendingRemote<TrustedURLLoaderHeaderClient> outer,
    mojo::PendingRemote<TrustedURLLoaderHeaderClient> inner,
    HandlesLoaderCallback inner_handles_loader) {
  new URLLoaderHeaderClientChain(std::move(receiver), std::move(outer),
                                 std::move(inner),
                                 std::move(inner_handles_loader));
}

}  // namespace electron
