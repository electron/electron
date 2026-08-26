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

RequestObserverTarget::RequestObserverTarget() = default;
RequestObserverTarget::RequestObserverTarget(const RequestObserverTarget&) =
    default;
RequestObserverTarget::~RequestObserverTarget() = default;

namespace {

base::AtomicSequenceNumber g_observed_request_key;

scoped_refptr<base::SequencedTaskRunner> UIThread() {
  return content::GetUIThreadTaskRunner({});
}

// Sits on a request's URLLoaderClient pipe on the IO thread, passing every
// message straight to the renderer and telling api::WebRequest afterwards.
class ObservedLoad : public network::mojom::URLLoaderClient {
 public:
  ObservedLoad(const RequestObserverTarget& target,
               int32_t request_id,
               const network::ResourceRequest& request,
               mojo::PendingReceiver<network::mojom::URLLoaderClient> receiver,
               mojo::PendingRemote<network::mojom::URLLoaderClient> renderer)
      : browser_context_(target.browser_context),
        key_(g_observed_request_key.GetNext()),
        receiver_(this, std::move(receiver)),
        renderer_(std::move(renderer)) {
    receiver_.set_disconnect_handler(base::BindOnce(
        &ObservedLoad::Finish, base::Unretained(this), std::nullopt));
    renderer_.set_disconnect_handler(base::BindOnce(
        &ObservedLoad::Finish, base::Unretained(this), std::nullopt));
    UIThread()->PostTask(
        FROM_HERE,
        base::BindOnce(&api::WebRequest::ObservedRequestStarted,
                       browser_context_, key_, target.render_process_id,
                       target.frame_routing_id, request_id, request));
  }
  ~ObservedLoad() override = default;

 private:
  // network::mojom::URLLoaderClient:
  void OnReceiveEarlyHints(network::mojom::EarlyHintsPtr early_hints) override {
    renderer_->OnReceiveEarlyHints(std::move(early_hints));
  }
  void OnReceiveResponse(
      network::mojom::URLResponseHeadPtr head,
      mojo::ScopedDataPipeConsumerHandle body,
      std::optional<mojo_base::BigBuffer> cached_metadata) override {
    auto head_for_ui = head.Clone();
    renderer_->OnReceiveResponse(std::move(head), std::move(body),
                                 std::move(cached_metadata));
    UIThread()->PostTask(
        FROM_HERE,
        base::BindOnce(&api::WebRequest::ObservedRequestResponded,
                       browser_context_, key_, std::move(head_for_ui)));
  }
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         network::mojom::URLResponseHeadPtr head) override {
    auto head_for_ui = head.Clone();
    renderer_->OnReceiveRedirect(redirect_info, std::move(head));
    UIThread()->PostTask(
        FROM_HERE, base::BindOnce(&api::WebRequest::ObservedRequestRedirected,
                                  browser_context_, key_, redirect_info,
                                  std::move(head_for_ui)));
  }
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback callback) override {
    renderer_->OnUploadProgress(current_position, total_size,
                                std::move(callback));
  }
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override {
    renderer_->OnTransferSizeUpdated(transfer_size_diff);
  }
  void OnComplete(const network::URLLoaderCompletionStatus& status) override {
    renderer_->OnComplete(status);
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
  const uint64_t key_;
  mojo::Receiver<network::mojom::URLLoaderClient> receiver_;
  mojo::Remote<network::mojom::URLLoaderClient> renderer_;
};

class URLLoaderFactoryGate : public network::SelfDeletingURLLoaderFactory {
 public:
  URLLoaderFactoryGate(
      scoped_refptr<InterceptState> state,
      RequestObserverTarget observer_target,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> interceptor,
      base::SelfDeletingPassKey key)
      : network::SelfDeletingURLLoaderFactory(std::move(receiver), key),
        state_(std::move(state)),
        observer_target_(std::move(observer_target)),
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
        client = ObserveRequest(observer_target_, request_id, request,
                                std::move(client));
        [[fallthrough]];
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
  const RequestObserverTarget observer_target_;
  mojo::Remote<network::mojom::URLLoaderFactory> target_;
  mojo::Remote<network::mojom::URLLoaderFactory> interceptor_;
};

}  // namespace

mojo::PendingRemote<network::mojom::URLLoaderClient> ObserveRequest(
    const RequestObserverTarget& target,
    int32_t request_id,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client) {
  mojo::PendingRemote<network::mojom::URLLoaderClient> observer;
  auto receiver = observer.InitWithNewPipeAndPassReceiver();
  auto create =
      [](RequestObserverTarget target, int32_t request_id,
         network::ResourceRequest request,
         mojo::PendingReceiver<network::mojom::URLLoaderClient> receiver,
         mojo::PendingRemote<network::mojom::URLLoaderClient> client) {
        // Deletes itself when the request completes or either side goes away.
        new ObservedLoad(target, request_id, request, std::move(receiver),
                         std::move(client));
      };
  if (content::BrowserThread::CurrentlyOn(content::BrowserThread::IO)) {
    create(target, request_id, request, std::move(receiver), std::move(client));
  } else {
    content::GetIOThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(create, target, request_id, request,
                                  std::move(receiver), std::move(client)));
  }
  return observer;
}

void CreateURLLoaderFactoryGate(
    scoped_refptr<InterceptState> state,
    RequestObserverTarget observer_target,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> interceptor) {
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](scoped_refptr<InterceptState> state,
             RequestObserverTarget observer,
             mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
             mojo::PendingRemote<network::mojom::URLLoaderFactory> target,
             mojo::PendingRemote<network::mojom::URLLoaderFactory>
                 interceptor) {
            base::MakeSelfDeleting<URLLoaderFactoryGate>(
                std::move(state), std::move(observer), std::move(receiver),
                std::move(target), std::move(interceptor));
          },
          std::move(state), std::move(observer_target), std::move(receiver),
          std::move(target), std::move(interceptor)));
}

}  // namespace electron
