// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_BROWSER_CONTEXT_H_
#define BRIGHTRAY_BROWSER_BROWSER_CONTEXT_H_

#include "browser/url_request_context_getter.h"

#include "content/public/browser/browser_context.h"

class PrefRegistrySimple;
class PrefService;

namespace brightray {

class DownloadManagerDelegate;

class BrowserContext : public content::BrowserContext,
                       public brightray::URLRequestContextGetter::Delegate {
 public:
  BrowserContext();
  ~BrowserContext();

  virtual void Initialize();

  net::URLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector protocol_interceptors);

  net::URLRequestContextGetter* url_request_context_getter() const {
    return url_request_getter_.get();
  }

  PrefService* prefs() { return prefs_.get(); }

 protected:
  // Subclasses should override this to register custom preferences.
  virtual void RegisterPrefs(PrefRegistrySimple* pref_registry) {}

  // URLRequestContextGetter::Delegate:
  virtual net::NetworkDelegate* CreateNetworkDelegate() override;

  virtual base::FilePath GetPath() const override;

 private:
  class ResourceContext;

  void RegisterInternalPrefs(PrefRegistrySimple* pref_registry);

  virtual bool IsOffTheRecord() const override;
  virtual net::URLRequestContextGetter* GetRequestContext() override;
  virtual net::URLRequestContextGetter* GetRequestContextForRenderProcess(
      int renderer_child_id);
  virtual net::URLRequestContextGetter* GetMediaRequestContext() override;
  virtual net::URLRequestContextGetter* GetMediaRequestContextForRenderProcess(
      int renderer_child_id) override;
  virtual net::URLRequestContextGetter*
      GetMediaRequestContextForStoragePartition(
          const base::FilePath& partition_path, bool in_memory);
  virtual content::ResourceContext* GetResourceContext() override;
  virtual content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  virtual content::BrowserPluginGuestManager* GetGuestManager() override;
  virtual quota::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  virtual content::PushMessagingService* GetPushMessagingService() override;
  virtual content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;

  base::FilePath path_;
  scoped_ptr<ResourceContext> resource_context_;
  scoped_refptr<URLRequestContextGetter> url_request_getter_;
  scoped_ptr<PrefService> prefs_;
  scoped_ptr<DownloadManagerDelegate> download_manager_delegate_;

  DISALLOW_COPY_AND_ASSIGN(BrowserContext);
};

}  // namespace brightray

#endif
