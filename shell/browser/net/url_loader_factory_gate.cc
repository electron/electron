// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/url_loader_factory_gate.h"

#include <utility>

#include "base/functional/bind.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "url/gurl.h"

namespace electron {

InterceptState::InterceptState() = default;
InterceptState::~InterceptState() = default;

void InterceptState::SetHasWebRequestListeners(bool has_listeners) {
  base::AutoLock lock(lock_);
  has_web_request_listeners_ = has_listeners;
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

bool InterceptState::WantsRequest(const GURL& url) const {
  base::AutoLock lock(lock_);
  if (has_web_request_listeners_ || intercepted_schemes_.contains(url.scheme()))
    return true;
  for (const auto& domain : ignore_connections_limit_domains_) {
    if (url.DomainIs(domain))
      return true;
  }
  return false;
}

namespace {

class URLLoaderFactoryGate : public network::mojom::URLLoaderFactory {
 public:
  URLLoaderFactoryGate(
      scoped_refptr<InterceptState> state,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> interceptor)
      : state_(std::move(state)),
        target_(std::move(target)),
        interceptor_(std::move(interceptor)) {
    receivers_.Add(this, std::move(receiver));
    receivers_.set_disconnect_handler(base::BindRepeating(
        &URLLoaderFactoryGate::OnReceiverGone, base::Unretained(this)));
    target_.set_disconnect_handler(
        base::BindOnce(&URLLoaderFactoryGate::Destroy, base::Unretained(this)));
    interceptor_.set_disconnect_handler(
        base::BindOnce(&URLLoaderFactoryGate::Destroy, base::Unretained(this)));
  }
  ~URLLoaderFactoryGate() override = default;

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override {
    auto& factory = state_->WantsRequest(request.url) ? interceptor_ : target_;
    factory->CreateLoaderAndStart(std::move(loader), request_id, options,
                                  request, std::move(client),
                                  traffic_annotation);
  }
  void Clone(mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver)
      override {
    receivers_.Add(this, std::move(receiver));
  }

 private:
  void OnReceiverGone() {
    if (receivers_.empty())
      Destroy();
  }
  void Destroy() { delete this; }

  const scoped_refptr<InterceptState> state_;
  mojo::ReceiverSet<network::mojom::URLLoaderFactory> receivers_;
  mojo::Remote<network::mojom::URLLoaderFactory> target_;
  mojo::Remote<network::mojom::URLLoaderFactory> interceptor_;
};

}  // namespace

void CreateURLLoaderFactoryGate(
    scoped_refptr<InterceptState> state,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> interceptor) {
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](scoped_refptr<InterceptState> state,
             mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
             mojo::PendingRemote<network::mojom::URLLoaderFactory> target,
             mojo::PendingRemote<network::mojom::URLLoaderFactory>
                 interceptor) {
            new URLLoaderFactoryGate(std::move(state), std::move(receiver),
                                     std::move(target), std::move(interceptor));
          },
          std::move(state), std::move(receiver), std::move(target),
          std::move(interceptor)));
}

}  // namespace electron
