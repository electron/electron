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

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
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
class SimpleURLLoaderClient;

// Receives a request's response body from SimpleURLLoader. Chunks are handed
// to the delegate (JS) until Hold(); from then on they are kept, and RelayTo()
// streams the kept and all further chunks into a data pipe for another
// URLLoaderClient and completes that client once the request has completed.
class ResponseBody final : public network::SimpleURLLoaderStreamConsumer,
                           private network::mojom::URLLoader {
 public:
  class Delegate {
   public:
    virtual void OnBodyData(std::string_view chunk,
                            base::OnceClosure resume) = 0;
    // Returns the request's net error. Once held, the delegate only releases
    // the request here; the outcome reaches the relay client instead of JS.
    virtual int OnBodyComplete(bool success) = 0;
    // The relay client has been completed or went away.
    virtual void OnRelayDone() = 0;

   protected:
    virtual ~Delegate() = default;
  };

  explicit ResponseBody(Delegate* delegate);
  ~ResponseBody() override;

  void Hold();
  bool held() const { return held_; }
  // |prefix| is body data the delegate had already taken before Hold().
  void RelayTo(mojo::PendingRemote<network::mojom::URLLoaderClient> client,
               mojo::PendingReceiver<network::mojom::URLLoader> loader,
               mojo::ScopedDataPipeProducerHandle producer,
               std::string prefix);
  // Drops the relay client without completing it.
  void Abort();

  // network::SimpleURLLoaderStreamConsumer, fed by SimpleURLLoaderClient
  void OnDataReceived(std::string_view chunk,
                      base::OnceClosure resume) override;
  void OnComplete(bool success) override;
  void OnRetry(base::OnceClosure start_retry) override {}

 private:
  // network::mojom::URLLoader, towards the relay client
  void FollowRedirect(
      network::HttpRequestHeadersUpdateParams headers_update_params,
      const std::optional<GURL>& new_url) override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}

  size_t WriteSome(std::string_view bytes);
  void Pump();
  void OnWritable(MojoResult result);
  void Finish(int net_error);

  const raw_ptr<Delegate> delegate_;
  bool held_ = false;
  std::string pending_;
  base::OnceClosure resume_;
  std::optional<int> result_;
  size_t written_ = 0;
  mojo::ScopedDataPipeProducerHandle producer_;
  std::unique_ptr<mojo::SimpleWatcher> watcher_;
  mojo::Remote<network::mojom::URLLoaderClient> client_;
  mojo::Receiver<network::mojom::URLLoader> receiver_{this};
  base::WeakPtrFactory<ResponseBody> weak_factory_{this};
};

/** Wraps a SimpleURLLoader to make it usable from JavaScript */
class SimpleURLLoaderWrapper final
    : public gin::Wrappable<SimpleURLLoaderWrapper>,
      public gin_helper::EventEmitterMixin<SimpleURLLoaderWrapper>,
      public ResponseBody::Delegate {
 public:
  ~SimpleURLLoaderWrapper() override;
  static SimpleURLLoaderWrapper* Create(gin::Arguments* args);

  void Cancel();

  // For protocol.handle handlers that return a net.fetch response untouched:
  // stream the body to |client| instead of JS. See ResponseBody.
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
  friend class SimpleURLLoaderClient;

  void Hold();

  // ResponseBody::Delegate:
  void OnBodyData(std::string_view chunk, base::OnceClosure resume) override;
  int OnBodyComplete(bool success) override;
  void OnRelayDone() override;

  void OnAuthRequired(
      const std::optional<base::UnguessableToken>& window_id,
      int32_t request_id,
      const GURL& url,
      bool first_auth_attempt,
      const net::AuthChallengeInfo& auth_info,
      const scoped_refptr<net::HttpResponseHeaders>& head_headers,
      mojo::PendingRemote<network::mojom::AuthChallengeResponder>
          auth_challenge_responder);
  void OnCertificateRequested(
      const std::optional<base::UnguessableToken>& window_id,
      const scoped_refptr<net::SSLCertRequestInfo>& cert_info,
      mojo::PendingRemote<network::mojom::ClientCertificateResponder>
          client_cert_responder);

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

  SEQUENCE_CHECKER(sequence_checker_);
  raw_ptr<ElectronBrowserContext> browser_context_;
  int request_options_;
  std::unique_ptr<network::ResourceRequest> request_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  // The client receives callbacks from |loader_| and must outlive it.
  std::unique_ptr<SimpleURLLoaderClient> client_;
  std::unique_ptr<ResponseBody> body_;  // outlives |loader_|, its producer
  std::unique_ptr<network::SimpleURLLoader> loader_;
  cppgc::Member<JSChunkedDataPipeGetter> chunk_pipe_getter_;

  int64_t content_length_ = -1;
  gin_helper::SelfKeepAlive<SimpleURLLoaderWrapper> keep_alive_{this};
  gin::WeakCellFactory<SimpleURLLoaderWrapper> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_URL_LOADER_H_
