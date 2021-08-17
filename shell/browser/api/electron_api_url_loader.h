// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_URL_LOADER_H_
#define SHELL_BROWSER_API_ELECTRON_API_URL_LOADER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "net/base/auth.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "services/network/public/mojom/devtools_observer.mojom.h"
#include "services/network/public/mojom/http_raw_headers.mojom.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom-forward.h"
#include "services/network/public/mojom/url_loader_network_service_observer.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "shell/browser/event_emitter_mixin.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace gin {
class Arguments;
template <typename T>
class Handle;
}  // namespace gin

namespace network {
class SimpleURLLoader;
struct ResourceRequest;
}  // namespace network

namespace electron {

namespace api {

/** Wraps a SimpleURLLoader to make it usable from JavaScript */
class SimpleURLLoaderWrapper
    : public gin::Wrappable<SimpleURLLoaderWrapper>,
      public gin_helper::EventEmitterMixin<SimpleURLLoaderWrapper>,
      public network::SimpleURLLoaderStreamConsumer,
      public network::mojom::URLLoaderNetworkServiceObserver,
      public network::mojom::DevToolsObserver {
 public:
  ~SimpleURLLoaderWrapper() override;
  static gin::Handle<SimpleURLLoaderWrapper> Create(gin::Arguments* args);

  void Cancel();

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 private:
  SimpleURLLoaderWrapper(std::unique_ptr<network::ResourceRequest> request,
                         network::mojom::URLLoaderFactory* url_loader_factory,
                         int options);

  // SimpleURLLoaderStreamConsumer:
  void OnDataReceived(base::StringPiece string_piece,
                      base::OnceClosure resume) override;
  void OnComplete(bool success) override;
  void OnRetry(base::OnceClosure start_retry) override;

  // network::mojom::URLLoaderNetworkServiceObserver:
  void OnAuthRequired(
      const absl::optional<base::UnguessableToken>& window_id,
      uint32_t request_id,
      const GURL& url,
      bool first_auth_attempt,
      const net::AuthChallengeInfo& auth_info,
      const scoped_refptr<net::HttpResponseHeaders>& head_headers,
      mojo::PendingRemote<network::mojom::AuthChallengeResponder>
          auth_challenge_responder) override;
  void OnSSLCertificateError(const GURL& url,
                             int net_error,
                             const net::SSLInfo& ssl_info,
                             bool fatal,
                             OnSSLCertificateErrorCallback response) override;
  void OnCertificateRequested(
      const absl::optional<base::UnguessableToken>& window_id,
      const scoped_refptr<net::SSLCertRequestInfo>& cert_info,
      mojo::PendingRemote<network::mojom::ClientCertificateResponder>
          client_cert_responder) override {}
  void OnClearSiteData(const GURL& url,
                       const std::string& header_value,
                       int32_t load_flags,
                       OnClearSiteDataCallback callback) override;
  void OnLoadingStateUpdate(network::mojom::LoadInfoPtr info,
                            OnLoadingStateUpdateCallback callback) override;
  void OnDataUseUpdate(int32_t network_traffic_annotation_id_hash,
                       int64_t recv_bytes,
                       int64_t sent_bytes) override {}
  void Clone(
      mojo::PendingReceiver<network::mojom::URLLoaderNetworkServiceObserver>
          observer) override;

  // network::mojom::DevToolsObserver:
  void OnRawRequest(
      const std::string& devtools_request_id,
      const net::CookieAccessResultList& cookies_with_access_result,
      std::vector<network::mojom::HttpRawHeaderPairPtr> headers,
      const base::TimeTicks timestamp,
      network::mojom::ClientSecurityStatePtr client_security_state) override {}
  void OnRawResponse(
      const std::string& devtools_request_id,
      const net::CookieAndLineAccessResultList& cookies_with_access_result,
      std::vector<network::mojom::HttpRawHeaderPairPtr> headers,
      const absl::optional<std::string>& raw_response_headers,
      network::mojom::IPAddressSpace resource_address_space,
      int32_t http_status_code) override;
  void OnPrivateNetworkRequest(
      const absl::optional<std::string>& devtools_request_id,
      const GURL& url,
      bool is_warning,
      network::mojom::IPAddressSpace resource_address_space,
      network::mojom::ClientSecurityStatePtr client_security_state) override {}
  void OnCorsPreflightRequest(
      const base::UnguessableToken& devtool_request_id,
      const net::HttpRequestHeaders& request_headers,
      network::mojom::URLRequestDevToolsInfoPtr request_info,
      const GURL& initiator_url,
      const std::string& initiator_devtool_request_id) override {}
  void OnCorsPreflightResponse(
      const base::UnguessableToken& devtool_request_id,
      const GURL& url,
      network::mojom::URLResponseHeadPtr head) override {}
  void OnCorsPreflightRequestCompleted(
      const base::UnguessableToken& devtool_request_id,
      const network::URLLoaderCompletionStatus& status) override {}
  void OnTrustTokenOperationDone(
      const std::string& devtool_request_id,
      network::mojom::TrustTokenOperationResultPtr result) override {}
  void OnCorsError(const absl::optional<std::string>& devtool_request_id,
                   const absl::optional<::url::Origin>& initiator_origin,
                   const GURL& url,
                   const network::CorsErrorStatus& status) override {}
  void OnSubresourceWebBundleMetadata(const std::string& devtools_request_id,
                                      const std::vector<GURL>& urls) override {}
  void OnSubresourceWebBundleMetadataError(
      const std::string& devtools_request_id,
      const std::string& error_message) override {}
  void OnSubresourceWebBundleInnerResponse(
      const std::string& inner_request_devtools_id,
      const ::GURL& url,
      const absl::optional<std::string>& bundle_request_devtools_id) override {}
  void OnSubresourceWebBundleInnerResponseError(
      const std::string& inner_request_devtools_id,
      const ::GURL& url,
      const std::string& error_message,
      const absl::optional<std::string>& bundle_request_devtools_id) override {}
  void Clone(mojo::PendingReceiver<network::mojom::DevToolsObserver> observer)
      override;

  // SimpleURLLoader callbacks
  void OnResponseStarted(const GURL& final_url,
                         const network::mojom::URLResponseHead& response_head);
  void OnRedirect(const net::RedirectInfo& redirect_info,
                  const network::mojom::URLResponseHead& response_head,
                  std::vector<std::string>* removed_headers);
  void OnUploadProgress(uint64_t position, uint64_t total);
  void OnDownloadProgress(uint64_t current);

  void Start();
  void Pin();
  void PinBodyGetter(v8::Local<v8::Value>);

  std::string devtools_request_id_;
  std::vector<network::mojom::HttpRawHeaderPairPtr> raw_response_headers_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  v8::Global<v8::Value> pinned_wrapper_;
  v8::Global<v8::Value> pinned_chunk_pipe_getter_;

  mojo::ReceiverSet<network::mojom::URLLoaderNetworkServiceObserver>
      url_loader_network_observer_receivers_;
  mojo::ReceiverSet<network::mojom::DevToolsObserver>
      devtools_observer_receivers_;
  base::WeakPtrFactory<SimpleURLLoaderWrapper> weak_factory_{this};
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_URL_LOADER_H_
