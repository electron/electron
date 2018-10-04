// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_CONTEXT_GETTER_H_
#define ATOM_BROWSER_NET_URL_REQUEST_CONTEXT_GETTER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "content/public/browser/browser_context.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/public/mojom/network_service.mojom.h"

#if DCHECK_IS_ON()
#include "base/debug/leak_tracker.h"
#endif

namespace brightray {
class RequireCTDelegate;
}  // namespace brightray

namespace atom {

class AtomBrowserContext;
class AtomURLRequestJobFactory;
class ResourceContext;

class URLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  // net::URLRequestContextGetter:
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

  // Discard reference to URLRequestContext and inform observers to
  // shutdown. Must be called only on IO thread.
  void NotifyContextShuttingDown(std::unique_ptr<ResourceContext>);

  AtomURLRequestJobFactory* job_factory() const {
    return top_job_factory_.get();
  }

 private:
  friend class AtomBrowserContext;

  // Responsible for destroying URLRequestContextGetter
  // on the IO thread.
  class Handle {
   public:
    explicit Handle(base::WeakPtr<AtomBrowserContext> browser_context);
    ~Handle();

    scoped_refptr<URLRequestContextGetter> CreateMainRequestContextGetter(
        content::ProtocolHandlerMap* protocol_handlers,
        content::URLRequestInterceptorScopedVector protocol_interceptors);
    content::ResourceContext* GetResourceContext();
    scoped_refptr<URLRequestContextGetter> GetMainRequestContextGetter();
    network::mojom::NetworkContextPtr GetNetworkContext();

    void ShutdownOnUIThread();

   private:
    friend class URLRequestContextGetter;
    void LazyInitialize();

    scoped_refptr<URLRequestContextGetter> main_request_context_getter_;
    std::unique_ptr<ResourceContext> resource_context_;
    base::WeakPtr<AtomBrowserContext> browser_context_;
    // This is a NetworkContext interface that uses URLRequestContextGetter
    // NetworkContext, ownership is passed to StoragePartition when
    // CreateMainNetworkContext is called.
    network::mojom::NetworkContextPtr main_network_context_;
    // Request corresponding to |main_network_context_|. Ownership
    // is passed to network service.
    network::mojom::NetworkContextRequest main_network_context_request_;
    network::mojom::NetworkContextParamsPtr main_network_context_params_;
    bool initialized_;

    DISALLOW_COPY_AND_ASSIGN(Handle);
  };

  URLRequestContextGetter(
      URLRequestContextGetter::Handle* context_handle,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector protocol_interceptors);
  ~URLRequestContextGetter() override;

#if DCHECK_IS_ON()
  base::debug::LeakTracker<URLRequestContextGetter> leak_tracker_;
#endif

  std::unique_ptr<brightray::RequireCTDelegate> ct_delegate_;
  std::unique_ptr<AtomURLRequestJobFactory> top_job_factory_;
  std::unique_ptr<network::mojom::NetworkContext> network_context_;

  URLRequestContextGetter::Handle* context_handle_;
  net::URLRequestContext* url_request_context_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector protocol_interceptors_;
  bool context_shutting_down_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestContextGetter);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_CONTEXT_GETTER_H_
