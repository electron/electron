// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_context.h"

#include <utility>

#include "atom/browser/atom_blob_reader.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_download_manager_delegate.h"
#include "atom/browser/atom_permission_manager.h"
#include "atom/browser/browser.h"
#include "atom/browser/cookie_change_notifier.h"
#include "atom/browser/special_storage_policy.h"
#include "atom/browser/web_view_manager.h"
#include "atom/common/atom_version.h"
#include "atom/common/chrome_version.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_restrictions.h"
#include "brightray/browser/brightray_paths.h"
#include "brightray/browser/inspectable_web_contents_impl.h"
#include "brightray/browser/zoom_level_delegate.h"
#include "brightray/common/application_info.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/user_agent.h"
#include "net/base/escape.h"

using content::BrowserThread;

namespace atom {

namespace {

std::string RemoveWhitespace(const std::string& str) {
  std::string trimmed;
  if (base::RemoveChars(str, " ", &trimmed))
    return trimmed;
  else
    return str;
}

// Convert string to lower case and escape it.
std::string MakePartitionName(const std::string& input) {
  return net::EscapePath(base::ToLowerASCII(input));
}

}  // namespace

// static
AtomBrowserContext::BrowserContextMap AtomBrowserContext::browser_context_map_;

// static
void AtomBrowserContextDeleter::Destruct(
    const AtomBrowserContext* browser_context) {
  browser_context->OnDestruct();
}

AtomBrowserContext::AtomBrowserContext(const std::string& partition,
                                       bool in_memory,
                                       const base::DictionaryValue& options)
    : storage_policy_(new SpecialStoragePolicy), weak_factory_(this) {
  // Construct user agent string.
  Browser* browser = Browser::Get();
  std::string name = RemoveWhitespace(browser->GetName());
  std::string user_agent;
  if (name == ATOM_PRODUCT_NAME) {
    user_agent = "Chrome/" CHROME_VERSION_STRING " " ATOM_PRODUCT_NAME
                 "/" ATOM_VERSION_STRING;
  } else {
    user_agent = base::StringPrintf(
        "%s/%s Chrome/%s " ATOM_PRODUCT_NAME "/" ATOM_VERSION_STRING,
        name.c_str(), browser->GetVersion().c_str(), CHROME_VERSION_STRING);
  }
  user_agent_ = content::BuildUserAgentFromProduct(user_agent);

  // Read options.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  use_cache_ = !command_line->HasSwitch(switches::kDisableHttpCache);
  options.GetBoolean("cache", &use_cache_);

  base::StringToInt(command_line->GetSwitchValueASCII(switches::kDiskCacheSize),
                    &max_cache_size_);

  if (!PathService::Get(brightray::DIR_USER_DATA, &path_)) {
    PathService::Get(brightray::DIR_APP_DATA, &path_);
    path_ = path_.Append(
        base::FilePath::FromUTF8Unsafe(brightray::GetApplicationName()));
    PathService::Override(brightray::DIR_USER_DATA, path_);
  }

  if (!in_memory && !partition.empty())
    path_ = path_.Append(FILE_PATH_LITERAL("Partitions"))
                .Append(base::FilePath::FromUTF8Unsafe(
                    MakePartitionName(partition)));

  content::BrowserContext::Initialize(this, path_);

  io_handle_ = new URLRequestContextGetter::Handle(weak_factory_.GetWeakPtr());

  browser_context_map_[PartitionKey(partition, in_memory)] =
      weak_factory_.GetWeakPtr();

  // Initialize Pref Registry.
  InitPrefs();

  cookie_change_notifier_ = std::make_unique<CookieChangeNotifier>(this);
}

AtomBrowserContext::~AtomBrowserContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  NotifyWillBeDestroyed(this);
  ShutdownStoragePartitions();
  io_handle_->ShutdownOnUIThread();
}

void AtomBrowserContext::OnDestruct() const {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    delete this;
  } else {
    BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
  }
}

void AtomBrowserContext::InitPrefs() {
  auto prefs_path = GetPath().Append(FILE_PATH_LITERAL("Preferences"));
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  PrefServiceFactory prefs_factory;
  scoped_refptr<JsonPrefStore> pref_store =
      base::MakeRefCounted<JsonPrefStore>(prefs_path);
  pref_store->ReadPrefs();  // Synchronous.
  prefs_factory.set_user_prefs(pref_store);

  auto registry = WrapRefCounted(new PrefRegistrySimple);

  registry->RegisterFilePathPref(prefs::kSelectFileLastDirectory,
                                 base::FilePath());
  base::FilePath download_dir;
  PathService::Get(chrome::DIR_DEFAULT_DOWNLOADS, &download_dir);
  registry->RegisterFilePathPref(prefs::kDownloadDefaultDirectory,
                                 download_dir);
  registry->RegisterDictionaryPref(prefs::kDevToolsFileSystemPaths);
  brightray::InspectableWebContentsImpl::RegisterPrefs(registry.get());
  brightray::MediaDeviceIDSalt::RegisterPrefs(registry.get());
  brightray::ZoomLevelDelegate::RegisterPrefs(registry.get());

  prefs_ = prefs_factory.Create(registry.get());
}

void AtomBrowserContext::SetUserAgent(const std::string& user_agent) {
  user_agent_ = user_agent;
}

net::URLRequestContextGetter* AtomBrowserContext::CreateRequestContext(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector protocol_interceptors) {
  return io_handle_
      ->CreateMainRequestContextGetter(protocol_handlers,
                                       std::move(protocol_interceptors))
      .get();
}

net::URLRequestContextGetter* AtomBrowserContext::CreateMediaRequestContext() {
  return io_handle_->GetMainRequestContextGetter().get();
}

net::URLRequestContextGetter* AtomBrowserContext::GetRequestContext() {
  return GetDefaultStoragePartition(this)->GetURLRequestContext();
}

network::mojom::NetworkContextPtr AtomBrowserContext::GetNetworkContext() {
  return io_handle_->GetNetworkContext();
}

base::FilePath AtomBrowserContext::GetPath() const {
  return path_;
}

bool AtomBrowserContext::IsOffTheRecord() const {
  return in_memory_;
}

bool AtomBrowserContext::CanUseHttpCache() const {
  return use_cache_;
}

int AtomBrowserContext::GetMaxCacheSize() const {
  return max_cache_size_;
}

content::ResourceContext* AtomBrowserContext::GetResourceContext() {
  return io_handle_->GetResourceContext();
}

std::string AtomBrowserContext::GetMediaDeviceIDSalt() {
  if (!media_device_id_salt_.get())
    media_device_id_salt_.reset(new brightray::MediaDeviceIDSalt(prefs_.get()));
  return media_device_id_salt_->GetSalt();
}

std::unique_ptr<content::ZoomLevelDelegate>
AtomBrowserContext::CreateZoomLevelDelegate(
    const base::FilePath& partition_path) {
  if (!IsOffTheRecord()) {
    return std::make_unique<brightray::ZoomLevelDelegate>(prefs(),
                                                          partition_path);
  }
  return std::unique_ptr<content::ZoomLevelDelegate>();
}

content::DownloadManagerDelegate*
AtomBrowserContext::GetDownloadManagerDelegate() {
  if (!download_manager_delegate_.get()) {
    auto* download_manager = content::BrowserContext::GetDownloadManager(this);
    download_manager_delegate_.reset(
        new AtomDownloadManagerDelegate(download_manager));
  }
  return download_manager_delegate_.get();
}

content::BrowserPluginGuestManager* AtomBrowserContext::GetGuestManager() {
  if (!guest_manager_)
    guest_manager_.reset(new WebViewManager);
  return guest_manager_.get();
}

content::PermissionManager* AtomBrowserContext::GetPermissionManager() {
  if (!permission_manager_.get())
    permission_manager_.reset(new AtomPermissionManager);
  return permission_manager_.get();
}

storage::SpecialStoragePolicy* AtomBrowserContext::GetSpecialStoragePolicy() {
  return storage_policy_.get();
}

std::string AtomBrowserContext::GetUserAgent() const {
  return user_agent_;
}

AtomBlobReader* AtomBrowserContext::GetBlobReader() {
  if (!blob_reader_.get()) {
    content::ChromeBlobStorageContext* blob_context =
        content::ChromeBlobStorageContext::GetFor(this);
    blob_reader_.reset(new AtomBlobReader(blob_context));
  }
  return blob_reader_.get();
}

content::PushMessagingService* AtomBrowserContext::GetPushMessagingService() {
  return nullptr;
}

content::SSLHostStateDelegate* AtomBrowserContext::GetSSLHostStateDelegate() {
  return nullptr;
}

content::BackgroundFetchDelegate*
AtomBrowserContext::GetBackgroundFetchDelegate() {
  return nullptr;
}

content::BackgroundSyncController*
AtomBrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

content::BrowsingDataRemoverDelegate*
AtomBrowserContext::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

net::URLRequestContextGetter*
AtomBrowserContext::CreateRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  NOTREACHED();
  return nullptr;
}

net::URLRequestContextGetter*
AtomBrowserContext::CreateMediaRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory) {
  NOTREACHED();
  return nullptr;
}

// static
scoped_refptr<AtomBrowserContext> AtomBrowserContext::From(
    const std::string& partition,
    bool in_memory,
    const base::DictionaryValue& options) {
  PartitionKey key(partition, in_memory);
  if (browser_context_map_[key].get())
    return WrapRefCounted(browser_context_map_[key].get());

  return new AtomBrowserContext(partition, in_memory, options);
}

}  // namespace atom
