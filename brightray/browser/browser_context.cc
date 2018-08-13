// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brightray/browser/browser_context.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "brightray/browser/brightray_paths.h"
#include "brightray/browser/browser_client.h"
#include "brightray/browser/inspectable_web_contents_impl.h"
#include "brightray/browser/special_storage_policy.h"
#include "brightray/browser/zoom_level_delegate.h"
#include "brightray/common/application_info.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/escape.h"

using content::BrowserThread;

namespace brightray {

namespace {

// Convert string to lower case and escape it.
std::string MakePartitionName(const std::string& input) {
  return net::EscapePath(base::ToLowerASCII(input));
}

}  // namespace

// static
void BrowserContextDeleter::Destruct(const BrowserContext* browser_context) {
  browser_context->OnDestruct();
}

// static
BrowserContext::BrowserContextMap BrowserContext::browser_context_map_;

// static
scoped_refptr<BrowserContext> BrowserContext::Get(const std::string& partition,
                                                  bool in_memory) {
  PartitionKey key(partition, in_memory);
  if (browser_context_map_[key].get())
    return WrapRefCounted(browser_context_map_[key].get());

  return nullptr;
}

BrowserContext::BrowserContext(const std::string& partition, bool in_memory)
    : in_memory_(in_memory),
      storage_policy_(new SpecialStoragePolicy),
      weak_factory_(this) {
  if (!PathService::Get(DIR_USER_DATA, &path_)) {
    PathService::Get(DIR_APP_DATA, &path_);
    path_ = path_.Append(base::FilePath::FromUTF8Unsafe(GetApplicationName()));
    PathService::Override(DIR_USER_DATA, path_);
  }

  if (!in_memory_ && !partition.empty())
    path_ = path_.Append(FILE_PATH_LITERAL("Partitions"))
                .Append(base::FilePath::FromUTF8Unsafe(
                    MakePartitionName(partition)));

  content::BrowserContext::Initialize(this, path_);

  io_handle_ = new URLRequestContextGetter::Handle(GetWeakPtr());

  browser_context_map_[PartitionKey(partition, in_memory)] = GetWeakPtr();
}

BrowserContext::~BrowserContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  NotifyWillBeDestroyed(this);
  ShutdownStoragePartitions();
  io_handle_->ShutdownOnUIThread();
}

void BrowserContext::OnDestruct() const {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    delete this;
  } else {
    BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
  }
}

void BrowserContext::InitPrefs() {
  auto prefs_path = GetPath().Append(FILE_PATH_LITERAL("Preferences"));
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  PrefServiceFactory prefs_factory;
  scoped_refptr<JsonPrefStore> pref_store =
      base::MakeRefCounted<JsonPrefStore>(prefs_path);
  pref_store->ReadPrefs();  // Synchronous.
  prefs_factory.set_user_prefs(pref_store);

  auto registry = WrapRefCounted(new PrefRegistrySimple);
  RegisterInternalPrefs(registry.get());
  RegisterPrefs(registry.get());

  prefs_ = prefs_factory.Create(registry.get());
}

void BrowserContext::RegisterInternalPrefs(PrefRegistrySimple* registry) {
  InspectableWebContentsImpl::RegisterPrefs(registry);
  MediaDeviceIDSalt::RegisterPrefs(registry);
  ZoomLevelDelegate::RegisterPrefs(registry);
}

URLRequestContextGetter* BrowserContext::GetRequestContext() {
  return static_cast<URLRequestContextGetter*>(
      GetDefaultStoragePartition(this)->GetURLRequestContext());
}

net::URLRequestContextGetter* BrowserContext::CreateRequestContext(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector protocol_interceptors) {
  return io_handle_
      ->CreateMainRequestContextGetter(protocol_handlers,
                                       std::move(protocol_interceptors))
      .get();
}

std::string BrowserContext::GetMediaDeviceIDSalt() {
  if (!media_device_id_salt_.get())
    media_device_id_salt_.reset(new MediaDeviceIDSalt(prefs_.get()));
  return media_device_id_salt_->GetSalt();
}

base::FilePath BrowserContext::GetPath() const {
  return path_;
}

std::unique_ptr<content::ZoomLevelDelegate>
BrowserContext::CreateZoomLevelDelegate(const base::FilePath& partition_path) {
  if (!IsOffTheRecord()) {
    return std::make_unique<ZoomLevelDelegate>(prefs(), partition_path);
  }
  return std::unique_ptr<content::ZoomLevelDelegate>();
}

bool BrowserContext::IsOffTheRecord() const {
  return in_memory_;
}

content::ResourceContext* BrowserContext::GetResourceContext() {
  return io_handle_->GetResourceContext();
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

content::BackgroundFetchDelegate* BrowserContext::GetBackgroundFetchDelegate() {
  return nullptr;
}

content::BackgroundSyncController*
BrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

content::BrowsingDataRemoverDelegate*
BrowserContext::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

net::URLRequestContextGetter*
BrowserContext::CreateRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  NOTREACHED();
  return nullptr;
}

net::URLRequestContextGetter* BrowserContext::CreateMediaRequestContext() {
  return io_handle_->GetMainRequestContextGetter().get();
}

net::URLRequestContextGetter*
BrowserContext::CreateMediaRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory) {
  NOTREACHED();
  return nullptr;
}

}  // namespace brightray
