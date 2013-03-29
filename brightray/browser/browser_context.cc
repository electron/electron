// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser_context.h"

#include "common/application_info.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/prefs/json_pref_store.h"
#include "base/prefs/pref_registry_simple.h"
#include "base/prefs/pref_service.h"
#include "base/prefs/pref_service_builder.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "url_request_context_getter.h"

namespace brightray {

class BrowserContext::ResourceContext : public content::ResourceContext {
public:
  ResourceContext() : getter_(nullptr) {}

  void set_url_request_context_getter(URLRequestContextGetter* getter) {
    getter_ = getter;
  }

private:
  virtual net::HostResolver* GetHostResolver() OVERRIDE {
    return getter_->host_resolver();
  }
  
  virtual net::URLRequestContext* GetRequestContext() OVERRIDE {
    return getter_->GetURLRequestContext();
  }

  URLRequestContextGetter* getter_;
};

BrowserContext::BrowserContext() : resource_context_(new ResourceContext) {
  auto prefs_path = GetPath().Append("Preferences");
  PrefServiceBuilder builder;
  builder.WithUserFilePrefs(prefs_path,
      JsonPrefStore::GetTaskRunnerForFile(prefs_path, content::BrowserThread::GetBlockingPool()));

  auto registry = make_scoped_refptr(new PrefRegistrySimple);
  RegisterPrefs(registry);

  prefs_.reset(builder.Create(registry));
}

BrowserContext::~BrowserContext() {
}

net::URLRequestContextGetter* BrowserContext::CreateRequestContext(content::ProtocolHandlerMap* protocol_handlers) {
  DCHECK(!url_request_getter_);
  url_request_getter_ = new URLRequestContextGetter(
      GetPath(),
      content::BrowserThread::UnsafeGetMessageLoopForThread(content::BrowserThread::IO),
      content::BrowserThread::UnsafeGetMessageLoopForThread(content::BrowserThread::FILE),
      protocol_handlers);
  resource_context_->set_url_request_context_getter(url_request_getter_.get());
  return url_request_getter_.get();
}

base::FilePath BrowserContext::GetPath() {
  base::FilePath path;
  CHECK(PathService::Get(base::DIR_APP_DATA, &path));
  return path.Append(GetApplicationName());
}

bool BrowserContext::IsOffTheRecord() const {
  return false;
}

net::URLRequestContextGetter* BrowserContext::GetRequestContext() {
  return GetDefaultStoragePartition(this)->GetURLRequestContext();
}

net::URLRequestContextGetter* BrowserContext::GetRequestContextForRenderProcess(int renderer_child_id) {
  return GetRequestContext();
}

net::URLRequestContextGetter* BrowserContext::GetMediaRequestContext() {
  return GetRequestContext();
}

net::URLRequestContextGetter* BrowserContext::GetMediaRequestContextForRenderProcess(int renderer_child_id) {
  return GetRequestContext();
}

net::URLRequestContextGetter* BrowserContext::GetMediaRequestContextForStoragePartition(const base::FilePath& partition_path, bool in_memory) {
  return GetRequestContext();
}

content::ResourceContext* BrowserContext::GetResourceContext() {
  return resource_context_.get();
}

content::DownloadManagerDelegate* BrowserContext::GetDownloadManagerDelegate() {
  return nullptr;
}

content::GeolocationPermissionContext* BrowserContext::GetGeolocationPermissionContext() {
  return nullptr;
}

content::SpeechRecognitionPreferences* BrowserContext::GetSpeechRecognitionPreferences() {
  return nullptr;
}

quota::SpecialStoragePolicy* BrowserContext::GetSpecialStoragePolicy() {
  return nullptr;
}

}
