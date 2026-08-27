// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/url_loader_factory_gate.h"

#include <optional>
#include <utility>

#include "base/atomic_sequence_num.h"
#include "base/functional/bind.h"
#include "base/memory/self_deleting.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/api/web_request/web_request_resource_type.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/url_request/redirect_util.h"
#include "services/network/public/cpp/http_request_headers_update_params.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/self_deleting_url_loader_factory.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "shell/browser/api/electron_api_web_request.h"
#include "shell/browser/electron_browser_context.h"
#include "url/gurl.h"

namespace electron {

using extensions::WebRequestResourceType;

// Mirrors ToWebRequestResourceType() in extensions/browser/api/web_request/
// web_request_info.cc, which is not exported.
WebRequestResourceType ResourceTypeOf(const network::ResourceRequest& request) {
  using Destination = network::mojom::RequestDestination;
  if (request.url.SchemeIsWSOrWSS())
    return WebRequestResourceType::WEB_SOCKET;
  if (request.is_fetch_like_api)
    return WebRequestResourceType::XHR;
  switch (request.destination) {
    case Destination::kDocument:
      return WebRequestResourceType::MAIN_FRAME;
    case Destination::kIframe:
    case Destination::kFrame:
    case Destination::kFencedframe:
      return WebRequestResourceType::SUB_FRAME;
    case Destination::kStyle:
    case Destination::kXslt:
      return WebRequestResourceType::STYLESHEET;
    case Destination::kJson:
    case Destination::kScript:
    case Destination::kText:
    case Destination::kWorker:
    case Destination::kSharedWorker:
    case Destination::kServiceWorker:
      return WebRequestResourceType::SCRIPT;
    case Destination::kImage:
      return WebRequestResourceType::IMAGE;
    case Destination::kFont:
      return WebRequestResourceType::FONT;
    case Destination::kObject:
    case Destination::kEmbed:
      return WebRequestResourceType::OBJECT;
    case Destination::kAudio:
    case Destination::kTrack:
    case Destination::kVideo:
      return WebRequestResourceType::MEDIA;
    case Destination::kReport:
      return WebRequestResourceType::CSP_REPORT;
    case Destination::kWebBundle:
      return WebRequestResourceType::WEBBUNDLE;
    case Destination::kEmpty:
      return request.keepalive ? WebRequestResourceType::PING
                               : WebRequestResourceType::OTHER;
    default:
      return WebRequestResourceType::OTHER;
  }
}

InterceptState::InterceptState() = default;
InterceptState::~InterceptState() = default;

void InterceptState::SetListenerTypes(uint32_t blocking_types,
                                      uint32_t observer_types) {
  base::AutoLock lock(lock_);
  blocking_types_ = blocking_types;
  observer_types_ = observer_types;
}

void InterceptState::SetInterceptedSchemes(
    base::flat_set<std::string> schemes) {
  base::AutoLock lock(lock_);
  intercepted_schemes_ = std::move(schemes);
}

void InterceptState::SetIgnoreConnectionsLimitDomains(
    std::vector<std::string> domains) {
  base::AutoLock lock(lock_);
  ignore_connections_limit_domains_ = std::move(domains);
}

bool InterceptState::WantsURL(const GURL& url) const {
  if (intercepted_schemes_.contains(url.scheme()))
    return true;
  for (const auto& domain : ignore_connections_limit_domains_) {
    if (url.DomainIs(domain))
      return true;
  }
  return false;
}

InterceptState::Route InterceptState::RouteFor(
    const network::ResourceRequest& request) const {
  base::AutoLock lock(lock_);
  if (WantsURL(request.url))
    return Route::kProxy;
  const uint32_t type_bit = 1u << static_cast<int>(ResourceTypeOf(request));
  if (blocking_types_ & type_bit)
    return Route::kProxy;
  if (observer_types_ & type_bit)
    return Route::kObserve;
  return Route::kDirect;
}

namespace {

base::AtomicSequenceNumber g_observed_request_key;

scoped_refptr<base::SequencedTaskRunner> UIThread() {
  return content::GetUIThreadTaskRunner({});
}

// Proxies both sides of an observed request on the IO thread.
class ObservedURLLoader : public network::mojom::URLLoader,
                          public network::mojom::URLLoaderClient {
 public:
  ObservedURLLoader(
      base::WeakPtr<ElectronBrowserContext> browser_context,
      int render_process_id,
      int frame_routing_id,
      const network::ResourceRequest& request,
      mojo::PendingReceiver<network::mojom::URLLoader> renderer_loader,
      mojo::PendingRemote<network::mojom::URLLoaderClient> renderer_client,
      network::mojom::URLLoaderClientEndpointsPtr network_endpoints)
      : browser_context_(std::move(browser_context)),
        render_process_id_(render_process_id),
        frame_routing_id_(frame_routing_id),
        key_(g_observed_request_key.GetNext()),
        request_(request),
        renderer_loader_receiver_(this, std::move(renderer_loader)),
        renderer_client_(std::move(renderer_client)),
        network_loader_(std::move(network_endpoints->url_loader)),
        network_client_receiver_(
            this,
            std::move(network_endpoints->url_loader_client)) {
    auto on_disconnect = base::BindOnce(
        &ObservedURLLoader::Finish, weak_factory_.GetWeakPtr(), std::nullopt);
    renderer_loader_receiver_.set_disconnect_handler(base::BindOnce(
        &ObservedURLLoader::Finish, weak_factory_.GetWeakPtr(), std::nullopt));
    renderer_client_.set_disconnect_handler(std::move(on_disconnect));
    network_client_receiver_.set_disconnect_handler(base::BindOnce(
        &ObservedURLLoader::Finish, weak_factory_.GetWeakPtr(), std::nullopt));
    UIThread()->PostTask(
        FROM_HERE, base::BindOnce(&api::WebRequest::ObservedRequestStarted,
                                  browser_context_, key_, render_process_id_,
                                  frame_routing_id_, request_));
  }
  ~ObservedURLLoader() override = default;

 private:
  // network::mojom::URLLoader:
  void FollowRedirect(
      network::HttpRequestHeadersUpdateParams headers_update_params,
      const std::optional<GURL>& new_url) override {
    DCHECK(pending_redirect_);
    if (pending_redirect_) {
      bool should_clear_upload = false;
      net::RedirectUtil::UpdateHttpRequest(
          request_.url, request_.method, *pending_redirect_,
          headers_update_params.removed_headers,
          headers_update_params.modified_headers, &request_.headers,
          &should_clear_upload);
      request_.cors_exempt_headers.MergeFrom(
          headers_update_params.modified_cors_exempt_headers);
      for (const auto& removed_header : headers_update_params.removed_headers) {
        request_.cors_exempt_headers.RemoveHeader(removed_header);
      }
      request_.UpdateOnRedirect(*pending_redirect_);
      if (new_url)
        request_.url = *new_url;
      if (should_clear_upload)
        request_.request_body = nullptr;
      pending_redirect_.reset();
      UIThread()->PostTask(
          FROM_HERE,
          base::BindOnce(&api::WebRequest::ObservedRequestFollowedRedirect,
                         browser_context_, key_, request_));
    }
    network_loader_->FollowRedirect(std::move(headers_update_params), new_url);
  }

  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {
    network_loader_->SetPriority(priority, intra_priority_value);
  }

  // network::mojom::URLLoaderClient:
  void OnReceiveEarlyHints(network::mojom::EarlyHintsPtr early_hints) override {
    renderer_client_->OnReceiveEarlyHints(std::move(early_hints));
  }
  void OnReceiveResponse(
      network::mojom::URLResponseHeadPtr head,
      mojo::ScopedDataPipeConsumerHandle body,
      std::optional<mojo_base::BigBuffer> cached_metadata) override {
    auto head_for_ui = head.Clone();
    renderer_client_->OnReceiveResponse(std::move(head), std::move(body),
                                        std::move(cached_metadata));
    UIThread()->PostTask(
        FROM_HERE,
        base::BindOnce(&api::WebRequest::ObservedRequestResponded,
                       browser_context_, key_, std::move(head_for_ui)));
  }
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         network::mojom::URLResponseHeadPtr head) override {
    DCHECK(!pending_redirect_);
    pending_redirect_ = redirect_info;
    auto head_for_ui = head.Clone();
    renderer_client_->OnReceiveRedirect(redirect_info, std::move(head));
    UIThread()->PostTask(
        FROM_HERE, base::BindOnce(&api::WebRequest::ObservedRequestRedirected,
                                  browser_context_, key_, redirect_info,
                                  std::move(head_for_ui)));
  }
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback callback) override {
    renderer_client_->OnUploadProgress(current_position, total_size,
                                       std::move(callback));
  }
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override {
    renderer_client_->OnTransferSizeUpdated(transfer_size_diff);
  }
  void OnComplete(const network::URLLoaderCompletionStatus& status) override {
    renderer_client_->OnComplete(status);
    Finish(status);
  }

  void Finish(std::optional<network::URLLoaderCompletionStatus> status) {
    UIThread()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &api::WebRequest::ObservedRequestFinished, browser_context_, key_,
            status.value_or(
                network::URLLoaderCompletionStatus(net::ERR_ABORTED))));
    delete this;
  }

  const base::WeakPtr<ElectronBrowserContext> browser_context_;
  const int render_process_id_;
  const int frame_routing_id_;
  const uint64_t key_;
  network::ResourceRequest request_;
  std::optional<net::RedirectInfo> pending_redirect_;
  mojo::Receiver<network::mojom::URLLoader> renderer_loader_receiver_;
  mojo::Remote<network::mojom::URLLoaderClient> renderer_client_;
  mojo::Remote<network::mojom::URLLoader> network_loader_;
  mojo::Receiver<network::mojom::URLLoaderClient> network_client_receiver_;
  base::WeakPtrFactory<ObservedURLLoader> weak_factory_{this};
};

void CreateObservedLoaderAndStart(
    base::WeakPtr<ElectronBrowserContext> browser_context,
    int render_process_id,
    int frame_routing_id,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    network::mojom::URLLoaderFactory* target) {
  mojo::PendingRemote<network::mojom::URLLoader> network_loader;
  auto network_loader_receiver =
      network_loader.InitWithNewPipeAndPassReceiver();
  mojo::PendingRemote<network::mojom::URLLoaderClient> network_client;
  auto network_endpoints = network::mojom::URLLoaderClientEndpoints::New(
      std::move(network_loader),
      network_client.InitWithNewPipeAndPassReceiver());
  new ObservedURLLoader(std::move(browser_context), render_process_id,
                        frame_routing_id, request, std::move(loader),
                        std::move(client), std::move(network_endpoints));
  target->CreateLoaderAndStart(std::move(network_loader_receiver), request_id,
                               options, request, std::move(network_client),
                               traffic_annotation);
}

class URLLoaderFactoryGate : public network::SelfDeletingURLLoaderFactory {
 public:
  URLLoaderFactoryGate(
      scoped_refptr<InterceptState> state,
      base::WeakPtr<ElectronBrowserContext> browser_context,
      int render_process_id,
      int frame_routing_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> interceptor,
      base::SelfDeletingPassKey key)
      : network::SelfDeletingURLLoaderFactory(std::move(receiver), key),
        state_(std::move(state)),
        browser_context_(std::move(browser_context)),
        render_process_id_(render_process_id),
        frame_routing_id_(frame_routing_id),
        target_(std::move(target)),
        interceptor_(std::move(interceptor)) {
    target_.set_disconnect_handler(
        base::BindOnce(&URLLoaderFactoryGate::DisconnectReceiversAndDestroy,
                       base::Unretained(this)));
    interceptor_.set_disconnect_handler(
        base::BindOnce(&URLLoaderFactoryGate::DisconnectReceiversAndDestroy,
                       base::Unretained(this)));
  }

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override {
    switch (state_->RouteFor(request)) {
      case InterceptState::Route::kProxy:
        interceptor_->CreateLoaderAndStart(std::move(loader), request_id,
                                           options, request, std::move(client),
                                           traffic_annotation);
        return;
      case InterceptState::Route::kObserve:
        CreateObservedLoaderAndStart(
            browser_context_, render_process_id_, frame_routing_id_,
            std::move(loader), request_id, options, request, std::move(client),
            traffic_annotation, target_.get());
        return;
      case InterceptState::Route::kDirect:
        target_->CreateLoaderAndStart(std::move(loader), request_id, options,
                                      request, std::move(client),
                                      traffic_annotation);
        return;
    }
  }

 private:
  ~URLLoaderFactoryGate() override = default;

  const scoped_refptr<InterceptState> state_;
  const base::WeakPtr<ElectronBrowserContext> browser_context_;
  const int render_process_id_;
  const int frame_routing_id_;
  mojo::Remote<network::mojom::URLLoaderFactory> target_;
  mojo::Remote<network::mojom::URLLoaderFactory> interceptor_;
};

}  // namespace

void CreateObservedLoaderAndStartOnIO(
    base::WeakPtr<ElectronBrowserContext> browser_context,
    int render_process_id,
    int frame_routing_id,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target) {
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](base::WeakPtr<ElectronBrowserContext> browser_context,
             int render_process_id, int frame_routing_id,
             mojo::PendingReceiver<network::mojom::URLLoader> loader,
             int32_t request_id, uint32_t options,
             network::ResourceRequest request,
             mojo::PendingRemote<network::mojom::URLLoaderClient> client,
             net::MutableNetworkTrafficAnnotationTag traffic_annotation,
             mojo::PendingRemote<network::mojom::URLLoaderFactory> target) {
            mojo::Remote<network::mojom::URLLoaderFactory> target_remote(
                std::move(target));
            CreateObservedLoaderAndStart(
                std::move(browser_context), render_process_id, frame_routing_id,
                std::move(loader), request_id, options, request,
                std::move(client), traffic_annotation, target_remote.get());
          },
          std::move(browser_context), render_process_id, frame_routing_id,
          std::move(loader), request_id, options, request, std::move(client),
          traffic_annotation, std::move(target)));
}

void CreateURLLoaderFactoryGate(
    ElectronBrowserContext* browser_context,
    int render_process_id,
    int frame_routing_id,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> interceptor) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  scoped_refptr<InterceptState> state(browser_context->intercept_state());
  auto weak_browser_context = browser_context->GetWeakPtr();
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](scoped_refptr<InterceptState> state,
             base::WeakPtr<ElectronBrowserContext> browser_context,
             int render_process_id, int frame_routing_id,
             mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
             mojo::PendingRemote<network::mojom::URLLoaderFactory> target,
             mojo::PendingRemote<network::mojom::URLLoaderFactory>
                 interceptor) {
            base::MakeSelfDeleting<URLLoaderFactoryGate>(
                std::move(state), std::move(browser_context), render_process_id,
                frame_routing_id, std::move(receiver), std::move(target),
                std::move(interceptor));
          },
          std::move(state), std::move(weak_browser_context), render_process_id,
          frame_routing_id, std::move(receiver), std::move(target),
          std::move(interceptor)));
}

}  // namespace electron
