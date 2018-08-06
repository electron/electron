// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_URL_REQUEST_CONTEXT_GETTER_H_
#define BRIGHTRAY_BROWSER_URL_REQUEST_CONTEXT_GETTER_H_

#include "brightray/browser/net/url_request_context_getter_factory.h"
#include "content/public/browser/resource_context.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

#if DCHECK_IS_ON()
#include "base/debug/leak_tracker.h"
#endif

namespace brightray {

class BrowserContext;
class ResourceContext;

class URLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  URLRequestContextGetter(URLRequestContextGetterFactory* factory,
                          ResourceContext* resource_context);

  // net::URLRequestContextGetter:
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

  // Discard reference to URLRequestContext and inform observers to
  // shutdown. Must be called only on IO thread.
  void NotifyContextShuttingDown();

  net::URLRequestJobFactory* job_factory() { return factory_->job_factory(); }

 private:
  friend class BrowserContext;

  // Responsible for destroying URLRequestContextGetter
  // on the IO thread.
  class Handle {
   public:
    explicit Handle(base::WeakPtr<BrowserContext> browser_context);
    ~Handle();

    scoped_refptr<URLRequestContextGetter> CreateMainRequestContextGetter(
        URLRequestContextGetterFactory* factory);
    content::ResourceContext* GetResourceContext() const;
    scoped_refptr<URLRequestContextGetter> GetMediaRequestContextGetter() const;
    scoped_refptr<URLRequestContextGetter> GetMainRequestContextGetter() const {
      return main_request_context_getter_;
    }

    void ShutdownOnUIThread();

   private:
    void LazyInitialize() const;

    scoped_refptr<URLRequestContextGetter> main_request_context_getter_;
    scoped_refptr<URLRequestContextGetter> media_request_context_getter_;
    std::unique_ptr<ResourceContext> resource_context_;
    base::WeakPtr<BrowserContext> browser_context_;
    mutable bool initialized_;

    DISALLOW_COPY_AND_ASSIGN(Handle);
  };

  ~URLRequestContextGetter() override;

#if DCHECK_IS_ON()
  base::debug::LeakTracker<URLRequestContextGetter> leak_tracker_;
#endif

  std::unique_ptr<URLRequestContextGetterFactory> factory_;

  ResourceContext* resource_context_;
  net::URLRequestContext* url_request_context_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestContextGetter);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_URL_REQUEST_CONTEXT_GETTER_H_
