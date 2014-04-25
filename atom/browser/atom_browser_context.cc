// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_context.h"

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/net/atom_url_request_context_getter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_context.h"
#include "vendor/brightray/browser/network_delegate.h"

namespace atom {

using content::BrowserThread;

class AtomResourceContext : public content::ResourceContext {
 public:
  AtomResourceContext() : getter_(NULL) {}

  void set_url_request_context_getter(AtomURLRequestContextGetter* getter) {
    getter_ = getter;
  }

 protected:
  virtual net::HostResolver* GetHostResolver() OVERRIDE {
    DCHECK(getter_);
    return getter_->host_resolver();
  }

  virtual net::URLRequestContext* GetRequestContext() OVERRIDE {
    DCHECK(getter_);
    return getter_->GetURLRequestContext();
  }

  virtual bool AllowMicAccess(const GURL& origin) OVERRIDE {
    return true;
  }

  virtual bool AllowCameraAccess(const GURL& origin) OVERRIDE {
    return true;
  }

 private:
  AtomURLRequestContextGetter* getter_;

  DISALLOW_COPY_AND_ASSIGN(AtomResourceContext);
};

AtomBrowserContext::AtomBrowserContext()
    : resource_context_(new AtomResourceContext) {
}

AtomBrowserContext::~AtomBrowserContext() {
}

AtomURLRequestContextGetter* AtomBrowserContext::CreateRequestContext(
    content::ProtocolHandlerMap* protocol_handlers) {
  DCHECK(!url_request_getter_);
  url_request_getter_ = new AtomURLRequestContextGetter(
      GetPath(),
      BrowserThread::UnsafeGetMessageLoopForThread(BrowserThread::IO),
      BrowserThread::UnsafeGetMessageLoopForThread(BrowserThread::FILE),
      base::Bind(&AtomBrowserContext::CreateNetworkDelegate,
                 base::Unretained(this)),
      protocol_handlers);

  resource_context_->set_url_request_context_getter(url_request_getter_.get());
  return url_request_getter_.get();
}

content::ResourceContext* AtomBrowserContext::GetResourceContext() {
  return resource_context_.get();
}

// static
AtomBrowserContext* AtomBrowserContext::Get() {
  return static_cast<AtomBrowserContext*>(
      AtomBrowserMainParts::Get()->browser_context());
}

}  // namespace atom
