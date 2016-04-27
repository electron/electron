// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/browser_context.h"

#include "browser/brightray_paths.h"
#include "browser/inspectable_web_contents_impl.h"
#include "browser/network_delegate.h"
#include "browser/permission_manager.h"
#include "browser/special_storage_policy.h"
#include "common/application_info.h"

#include "base/files/file_path.h"
#include "base/path_service.h"

#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"

#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/escape.h"
#include "net/ssl/client_cert_store.h"

#if defined(USE_NSS_CERTS)
#include "net/ssl/client_cert_store_nss.h"
#elif defined(OS_WIN)
#include "net/ssl/client_cert_store_win.h"
#elif defined(OS_MACOSX)
#include "net/ssl/client_cert_store_mac.h"
#endif

using content::BrowserThread;

namespace brightray {

namespace {

// Convert string to lower case and escape it.
std::string MakePartitionName(const std::string& input) {
  return net::EscapePath(base::ToLowerASCII(input));
}

}  // namespace

class BrowserContext::ResourceContext : public content::ResourceContext {
 public:
  ResourceContext() : getter_(nullptr) {}

  void set_url_request_context_getter(URLRequestContextGetter* getter) {
    getter_ = getter;
  }

 private:
  net::HostResolver* GetHostResolver() override {
    return getter_->host_resolver();
  }

  net::URLRequestContext* GetRequestContext() override {
    return getter_->GetURLRequestContext();
  }

  scoped_ptr<net::ClientCertStore> CreateClientCertStore() override {
    #if defined(USE_NSS_CERTS)
      return scoped_ptr<net::ClientCertStore>(new net::ClientCertStoreNSS(
          net::ClientCertStoreNSS::PasswordDelegateFactory()));
    #elif defined(OS_WIN)
      return scoped_ptr<net::ClientCertStore>(new net::ClientCertStoreWin());
    #elif defined(OS_MACOSX)
      return scoped_ptr<net::ClientCertStore>(new net::ClientCertStoreMac());
    #elif defined(USE_OPENSSL)
      return scoped_ptr<net::ClientCertStore>();
    #endif
  }

  URLRequestContextGetter* getter_;
};

// static
BrowserContext::BrowserContextMap BrowserContext::browser_context_map_;

// static
scoped_refptr<BrowserContext> BrowserContext::From(
    const std::string& partition, bool in_memory) {
  PartitionKey key(partition, in_memory);
  if (browser_context_map_[key].get())
    return make_scoped_refptr(browser_context_map_[key].get());

  auto browser_context = BrowserContext::Create(partition, in_memory);
  browser_context->InitPrefs();
  browser_context_map_[key] = browser_context->weak_factory_.GetWeakPtr();
  return browser_context;
}

BrowserContext::BrowserContext(const std::string& partition, bool in_memory)
    : in_memory_(in_memory),
      resource_context_(new ResourceContext),
      storage_policy_(new SpecialStoragePolicy),
      weak_factory_(this) {
  if (!PathService::Get(DIR_USER_DATA, &path_)) {
    PathService::Get(DIR_APP_DATA, &path_);
    path_ = path_.Append(base::FilePath::FromUTF8Unsafe(GetApplicationName()));
    PathService::Override(DIR_USER_DATA, path_);
  }

  if (!in_memory_ && !partition.empty())
    path_ = path_.Append(FILE_PATH_LITERAL("Partitions"))
                 .Append(base::FilePath::FromUTF8Unsafe(MakePartitionName(partition)));
}

void BrowserContext::InitPrefs() {
  auto prefs_path = GetPath().Append(FILE_PATH_LITERAL("Preferences"));
  PrefServiceFactory prefs_factory;
  prefs_factory.SetUserPrefsFile(prefs_path,
      JsonPrefStore::GetTaskRunnerForFile(
          prefs_path, BrowserThread::GetBlockingPool()).get());

  auto registry = make_scoped_refptr(new PrefRegistrySimple);
  RegisterInternalPrefs(registry.get());
  RegisterPrefs(registry.get());

  prefs_ = prefs_factory.Create(registry.get());
}

BrowserContext::~BrowserContext() {
  BrowserThread::DeleteSoon(BrowserThread::IO,
                            FROM_HERE,
                            resource_context_.release());
}

void BrowserContext::RegisterInternalPrefs(PrefRegistrySimple* registry) {
  InspectableWebContentsImpl::RegisterPrefs(registry);
}

net::URLRequestContextGetter* BrowserContext::CreateRequestContext(
    NetLog* net_log,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector protocol_interceptors) {
  DCHECK(!url_request_getter_.get());
  url_request_getter_ = new URLRequestContextGetter(
      this,
      network_controller_handle(),
      net_log,
      GetPath(),
      in_memory_,
      BrowserThread::UnsafeGetMessageLoopForThread(BrowserThread::IO),
      BrowserThread::UnsafeGetMessageLoopForThread(BrowserThread::FILE),
      protocol_handlers,
      std::move(protocol_interceptors));
  resource_context_->set_url_request_context_getter(url_request_getter_.get());
  return url_request_getter_.get();
}

net::NetworkDelegate* BrowserContext::CreateNetworkDelegate() {
  return new NetworkDelegate;
}

base::FilePath BrowserContext::GetPath() const {
  return path_;
}

scoped_ptr<content::ZoomLevelDelegate> BrowserContext::CreateZoomLevelDelegate(
    const base::FilePath& partition_path) {
  return scoped_ptr<content::ZoomLevelDelegate>();
}

bool BrowserContext::IsOffTheRecord() const {
  return in_memory_;
}

net::URLRequestContextGetter* BrowserContext::GetRequestContext() {
  return GetDefaultStoragePartition(this)->GetURLRequestContext();
}

net::URLRequestContextGetter* BrowserContext::GetRequestContextForRenderProcess(
    int renderer_child_id) {
  return GetRequestContext();
}

net::URLRequestContextGetter* BrowserContext::GetMediaRequestContext() {
  return GetRequestContext();
}

net::URLRequestContextGetter*
    BrowserContext::GetMediaRequestContextForRenderProcess(
        int renderer_child_id) {
  return GetRequestContext();
}

net::URLRequestContextGetter*
    BrowserContext::GetMediaRequestContextForStoragePartition(
        const base::FilePath& partition_path,
        bool in_memory) {
  return GetRequestContext();
}

content::ResourceContext* BrowserContext::GetResourceContext() {
  return resource_context_.get();
}

content::DownloadManagerDelegate* BrowserContext::GetDownloadManagerDelegate() {
  return nullptr;
}

content::BrowserPluginGuestManager* BrowserContext::GetGuestManager() {
  return nullptr;
}

storage::SpecialStoragePolicy* BrowserContext::GetSpecialStoragePolicy() {
  return storage_policy_.get();
}

content::PushMessagingService* BrowserContext::GetPushMessagingService() {
  return nullptr;
}

content::SSLHostStateDelegate* BrowserContext::GetSSLHostStateDelegate() {
  return nullptr;
}

content::PermissionManager* BrowserContext::GetPermissionManager() {
  if (!permission_manager_.get())
    permission_manager_.reset(new PermissionManager);
  return permission_manager_.get();
}

content::BackgroundSyncController* BrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

}  // namespace brightray
