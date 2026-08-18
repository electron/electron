// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_URL_LOADER_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_URL_LOADER_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/byte_size.h"
#include "base/memory/raw_ptr.h"
#include "base/sequence_checker.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom-forward.h"
#include "services/network/public/mojom/url_loader_network_service_observer.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gc_plugin.h"
#include "shell/common/gin_helper/self_keep_alive.h"
#include "url/gurl.h"
#include "v8/include/cppgc/member.h"
#include "v8/include/v8-forward.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace net {
class AuthChallengeInfo;
}  // namespace net

namespace network {
class SimpleURLLoader;
struct ResourceRequest;
class SharedURLLoaderFactory;
}  // namespace network

namespace electron {
class ElectronBrowserContext;
}

namespace electron::api {

class JSChunkedDataPipeGetter;

/** Wraps a SimpleURLLoader to make it usable from JavaScript */
class SimpleURLLoaderWrapper final
    : public gin::Wrappable<SimpleURLLoaderWrapper>,
      public gin_helper::EventEmitterMixin<SimpleURLLoaderWrapper>,
      private network::SimpleURLLoaderStreamConsumer,
      private network::mojom::URLLoaderNetworkServiceObserver,
      private network::mojom::URLLoader {
 public:
  ~SimpleURLLoaderWrapper() override;
  static SimpleURLLoaderWrapper* Create(gin::Arguments* args);

  void Cancel();

  // Relay mode, for protocol.handle handlers that return a net.fetch response
  // untouched: Hold() stops handing body chunks to JS; RelayTo() then writes
  // |prefix| (the bytes JS had already pulled) and the rest of the body into
  // |producer| and completes |client|, with no further JS involvement.
  void Hold();
  void RelayTo(mojo::PendingRemote<network::mojom::URLLoaderClient> client,
               mojo::PendingReceiver<network::mojom::URLLoader> loader,
               mojo::ScopedDataPipeProducerHandle producer,
               std::string prefix);
  int64_t content_length() const { return content_length_; }

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  static const char* GetClassName() { return "SimpleURLLoaderWrapper"; }
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  void Trace(cppgc::Visitor* visitor) const override;

  SimpleURLLoaderWrapper(ElectronBrowserContext* browser_context,
                         std::unique_ptr<network::ResourceRequest> request,
                         int options,
                         JSChunkedDataPipeGetter* chunk_pipe_getter);

 private:
  // SimpleURLLoaderStreamConsumer:
  void OnDataReceived(std::string_view string_view,
                      base::OnceClosure resume) override;
  void OnComplete(bool success) override;
  void OnRetry(base::OnceClosure start_retry) override {}

  // network::mojom::URLLoaderNetworkServiceObserver:
  void OnAuthRequired(
      const std::optional<base::UnguessableToken>& window_id,
      int32_t request_id,
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
      const std::optional<base::UnguessableToken>& window_id,
      const scoped_refptr<net::SSLCertRequestInfo>& cert_info,
      mojo::PendingRemote<network::mojom::ClientCertificateResponder>
          client_cert_responder) override;
  void OnLocalNetworkAccessPermissionRequired(
      network::mojom::TransportType transport_type,
      network::mojom::IPAddressSpace ip_address_space,
      OnLocalNetworkAccessPermissionRequiredCallback callback) override {}
  void OnPlatformLocalNetworkPermissionRequired(
      OnPlatformLocalNetworkPermissionRequiredCallback callback) override;
  void OnClearSiteData(
      const GURL& url,
      const std::string& header_value,
      int32_t load_flags,
      const std::optional<net::CookiePartitionKey>& cookie_partition_key,
      bool partitioned_state_allowed_only,
      OnClearSiteDataCallback callback) override;
  void OnLoadingStateUpdate(network::mojom::LoadInfoPtr info,
                            OnLoadingStateUpdateCallback callback) override;
  void OnDataUseUpdate(int32_t network_traffic_annotation_id_hash,
                       base::ByteSize recv_bytes,
                       base::ByteSize sent_bytes) override {}
  void OnWebSocketConnectedToLocalNetwork(
      const GURL& request_url,
      network::mojom::IPAddressSpace ip_address_space) override {}
  void Clone(
      mojo::PendingReceiver<network::mojom::URLLoaderNetworkServiceObserver>
          observer) override;
  void OnUrlLoaderConnectedToLocalNetwork(
      const GURL& request_url,
      network::mojom::IPAddressSpace response_address_space,
      network::mojom::IPAddressSpace client_address_space,
      network::mojom::IPAddressSpace target_address_space) override {}

  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactoryForURL(
      const GURL& url);

  // SimpleURLLoader callbacks
  void OnResponseStarted(const GURL& final_url,
                         const network::mojom::URLResponseHead& response_head);
  void OnRedirect(const GURL& url_before_redirect,
                  const net::RedirectInfo& redirect_info,
                  const network::mojom::URLResponseHead& response_head,
                  std::vector<std::string>* removed_headers);
  void OnUploadProgress(uint64_t position, uint64_t total);
  void OnDownloadProgress(uint64_t current);

  void Start();

  // Relay mode helpers.
  size_t RelaySome(std::string_view bytes);
  void RelayWrite();
  void OnRelayWritable(MojoResult result);
  void FinishRelay(int net_error);
  // network::mojom::URLLoader (relay mode; the client end is a renderer):
  void FollowRedirect(
      network::HttpRequestHeadersUpdateParams headers_update_params,
      const std::optional<GURL>& new_url) override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}

  SEQUENCE_CHECKER(sequence_checker_);
  raw_ptr<ElectronBrowserContext> browser_context_;
  int request_options_;
  std::unique_ptr<network::ResourceRequest> request_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> loader_;

  GC_PLUGIN_IGNORE(
      "Context tracking of receivers is not needed in the browser process.")
  mojo::ReceiverSet<network::mojom::URLLoaderNetworkServiceObserver>
      url_loader_network_observer_receivers_;
  cppgc::Member<JSChunkedDataPipeGetter> chunk_pipe_getter_;

  int64_t content_length_ = -1;
  bool holding_ = false;
  std::string relay_pending_;
  size_t relay_written_ = 0;
  base::OnceClosure relay_resume_;
  std::optional<int> finished_;  // net error, once the request completed
  mojo::ScopedDataPipeProducerHandle relay_producer_;
  std::unique_ptr<mojo::SimpleWatcher> relay_watcher_;
  GC_PLUGIN_IGNORE("Browser-process mojo endpoints, no context tracking.")
  mojo::Remote<network::mojom::URLLoaderClient> relay_client_;
  GC_PLUGIN_IGNORE("Browser-process mojo endpoints, no context tracking.")
  mojo::Receiver<network::mojom::URLLoader> relay_receiver_{this};
  gin_helper::SelfKeepAlive<SimpleURLLoaderWrapper> keep_alive_{this};
  gin::WeakCellFactory<SimpleURLLoaderWrapper> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_URL_LOADER_H_
