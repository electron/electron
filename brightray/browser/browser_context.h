// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_BROWSER_CONTEXT_H_
#define BRIGHTRAY_BROWSER_BROWSER_CONTEXT_H_

#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"

class PrefRegistrySimple;
class PrefService;

namespace brightray {

class DownloadManagerDelegate;
class NetworkDelegate;
class URLRequestContextGetter;

class BrowserContext : public content::BrowserContext {
public:
  BrowserContext();
  ~BrowserContext();

  void Initialize();

  net::URLRequestContextGetter* CreateRequestContext(content::ProtocolHandlerMap*);

  PrefService* prefs() { return prefs_.get(); }

protected:
  // Subclasses should override this to register custom preferences.
  virtual void RegisterPrefs(PrefRegistrySimple*) {}

  // Subclasses should override this to provide a custom NetworkDelegate implementation.
  virtual scoped_ptr<NetworkDelegate> CreateNetworkDelegate();

  virtual base::FilePath GetPath() const OVERRIDE;

private:
  class ResourceContext;

  void RegisterInternalPrefs(PrefRegistrySimple*);

  virtual bool IsOffTheRecord() const OVERRIDE;
  virtual net::URLRequestContextGetter* GetRequestContext() OVERRIDE;
  virtual net::URLRequestContextGetter* GetRequestContextForRenderProcess(int renderer_child_id);
  virtual net::URLRequestContextGetter* GetMediaRequestContext() OVERRIDE;
  virtual net::URLRequestContextGetter* GetMediaRequestContextForRenderProcess(int renderer_child_id) OVERRIDE;
  virtual net::URLRequestContextGetter* GetMediaRequestContextForStoragePartition(const base::FilePath& partition_path, bool in_memory);
  virtual content::ResourceContext* GetResourceContext() OVERRIDE;
  virtual content::DownloadManagerDelegate* GetDownloadManagerDelegate() OVERRIDE;
  virtual content::GeolocationPermissionContext* GetGeolocationPermissionContext() OVERRIDE;
  virtual quota::SpecialStoragePolicy* GetSpecialStoragePolicy() OVERRIDE;

  base::FilePath path_;
  scoped_ptr<ResourceContext> resource_context_;
  scoped_refptr<URLRequestContextGetter> url_request_getter_;
  scoped_ptr<PrefService> prefs_;
  scoped_ptr<DownloadManagerDelegate> download_manager_delegate_;

  DISALLOW_COPY_AND_ASSIGN(BrowserContext);
};

}

#endif
