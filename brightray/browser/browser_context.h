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
      content::ProtocolHandlerScopedVector protocol_interceptors);

  net::URLRequestContextGetter* url_request_context_getter() const {
    return url_request_getter_.get();
  }

  PrefService* prefs() { return prefs_.get(); }

 protected:
  // Subclasses should override this to register custom preferences.
  virtual void RegisterPrefs(PrefRegistrySimple* pref_registry) {}

  // URLRequestContextGetter::Delegate:
  virtual net::NetworkDelegate* CreateNetworkDelegate() OVERRIDE;
  virtual net::URLRequestJobFactory* CreateURLRequestJobFactory(
      content::ProtocolHandlerMap* protocol_handlers,
      content::ProtocolHandlerScopedVector* protocol_interceptors) OVERRIDE;

  virtual base::FilePath GetPath() const OVERRIDE;

 private:
  class ResourceContext;

  void RegisterInternalPrefs(PrefRegistrySimple* pref_registry);

  virtual bool IsOffTheRecord() const OVERRIDE;
  virtual net::URLRequestContextGetter* GetRequestContext() OVERRIDE;
  virtual net::URLRequestContextGetter* GetRequestContextForRenderProcess(
      int renderer_child_id);
  virtual net::URLRequestContextGetter* GetMediaRequestContext() OVERRIDE;
  virtual net::URLRequestContextGetter* GetMediaRequestContextForRenderProcess(
      int renderer_child_id) OVERRIDE;
  virtual net::URLRequestContextGetter*
      GetMediaRequestContextForStoragePartition(
          const base::FilePath& partition_path, bool in_memory);
  virtual void RequestMidiSysExPermission(
      int render_process_id,
      int render_view_id,
      int bridge_id,
      const GURL& requesting_frame,
      bool user_gesture,
      const MidiSysExPermissionCallback& callback) OVERRIDE;
  virtual void CancelMidiSysExPermissionRequest(
      int render_process_id,
      int render_view_id,
      int bridge_id,
      const GURL& requesting_frame) OVERRIDE;
  virtual void RequestProtectedMediaIdentifierPermission(
      int render_process_id,
      int render_view_id,
      int bridge_id,
      int group_id,
      const GURL& requesting_frame,
      const ProtectedMediaIdentifierPermissionCallback& callback) OVERRIDE;
  virtual void CancelProtectedMediaIdentifierPermissionRequests(int group_id) OVERRIDE;
  virtual content::ResourceContext* GetResourceContext() OVERRIDE;
  virtual content::DownloadManagerDelegate*
      GetDownloadManagerDelegate() OVERRIDE;
  virtual content::GeolocationPermissionContext*
      GetGeolocationPermissionContext() OVERRIDE;
  virtual content::BrowserPluginGuestManagerDelegate*
      GetGuestManagerDelegate() OVERRIDE;
  virtual quota::SpecialStoragePolicy*
      GetSpecialStoragePolicy() OVERRIDE;

  base::FilePath path_;
  scoped_ptr<ResourceContext> resource_context_;
  scoped_refptr<URLRequestContextGetter> url_request_getter_;
  scoped_ptr<PrefService> prefs_;
  scoped_ptr<DownloadManagerDelegate> download_manager_delegate_;

  DISALLOW_COPY_AND_ASSIGN(BrowserContext);
};

}  // namespace brightray

#endif
