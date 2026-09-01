// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_URL_LOADER_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_URL_LOADER_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/sequence_checker.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "services/network/public/mojom/network_context.mojom.h"
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

/** Wraps a SimpleURLLoader to make it usable from JavaScript */
class SimpleURLLoaderWrapper final
    : public gin::Wrappable<SimpleURLLoaderWrapper>,
      public gin_helper::EventEmitterMixin<SimpleURLLoaderWrapper> {
 public:
  ~SimpleURLLoaderWrapper() override;
  static SimpleURLLoaderWrapper* Create(gin::Arguments* args);

  void Cancel();

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

  void OnDataReceived(std::string_view string_view, base::OnceClosure resume);
  void OnComplete(bool success);

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
  std::unique_ptr<network::SimpleURLLoader> loader_;
  cppgc::Member<JSChunkedDataPipeGetter> chunk_pipe_getter_;
  gin_helper::SelfKeepAlive<SimpleURLLoaderWrapper> keep_alive_{this};
  gin::WeakCellFactory<SimpleURLLoaderWrapper> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_URL_LOADER_H_
