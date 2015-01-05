// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_

#include "brightray/browser/browser_context.h"

class BrowserProcess;

namespace atom {

class AtomURLRequestJobFactory;
class WebViewManager;

class AtomBrowserContext : public brightray::BrowserContext {
 public:
  AtomBrowserContext();
  virtual ~AtomBrowserContext();

  // Returns the browser context singleton.
  static AtomBrowserContext* Get();

  // brightray::URLRequestContextGetter::Delegate:
  net::URLRequestJobFactory* CreateURLRequestJobFactory(
      content::ProtocolHandlerMap* handlers,
      content::URLRequestInterceptorScopedVector* interceptors) override;
  net::HttpCache::BackendFactory* CreateHttpCacheBackendFactory(
      const base::FilePath& base_path) override;

  // content::BrowserContext:
  content::BrowserPluginGuestManager* GetGuestManager() override;

  AtomURLRequestJobFactory* job_factory() const { return job_factory_; }

 private:
  // A fake BrowserProcess object that used to feed the source code from chrome.
  scoped_ptr<BrowserProcess> fake_browser_process_;
  scoped_ptr<WebViewManager> guest_manager_;

  AtomURLRequestJobFactory* job_factory_;  // Weak reference.

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserContext);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
