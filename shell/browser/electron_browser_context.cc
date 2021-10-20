// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_browser_context.h"

#include <memory>

#include <utility>

#include "base/barrier_closure.h"
#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/value_map_pref_store.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"  // nogncheck
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cors_origin_pattern_setter.h"
#include "content/public/browser/shared_cors_origin_access_list.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/escape.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "shell/browser/cookie_change_notifier.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/electron_download_manager_delegate.h"
#include "shell/browser/electron_permission_manager.h"
#include "shell/browser/net/resolve_proxy_helper.h"
#include "shell/browser/pref_store_delegate.h"
#include "shell/browser/protocol_registry.h"
#include "shell/browser/special_storage_policy.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/web_view_manager.h"
#include "shell/browser/zoom_level_delegate.h"
#include "shell/common/application_info.h"
#include "shell/common/electron_paths.h"
#include "shell/common/options_switches.h"

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/browser/browser_context_keyed_service_factories.h"
#include "extensions/browser/extension_pref_store.h"
#include "extensions/browser/extension_pref_value_map_factory.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/extension_api.h"
#include "shell/browser/extensions/electron_browser_context_keyed_service_factories.h"
#include "shell/browser/extensions/electron_extension_system.h"
#include "shell/browser/extensions/electron_extension_system_factory.h"
#include "shell/browser/extensions/electron_extensions_browser_client.h"
#include "shell/common/extensions/electron_extensions_client.h"
#endif  // BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS) || \
    BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/user_prefs/user_prefs.h"
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "base/i18n/rtl.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/spellcheck/browser/pref_names.h"
#include "components/spellcheck/common/spellcheck_common.h"
#endif

using content::BrowserThread;

namespace electron {

namespace {

// Convert string to lower case and escape it.
std::string MakePartitionName(const std::string& input) {
  return net::EscapePath(base::ToLowerASCII(input));
}

}  // namespace

// static
ElectronBrowserContext::BrowserContextMap&
ElectronBrowserContext::browser_context_map() {
  static base::NoDestructor<ElectronBrowserContext::BrowserContextMap>
      browser_context_map;
  return *browser_context_map;
}

ElectronBrowserContext::ElectronBrowserContext(const std::string& partition,
                                               bool in_memory,
                                               base::DictionaryValue options)
    : storage_policy_(base::MakeRefCounted<SpecialStoragePolicy>()),
      protocol_registry_(base::WrapUnique(new ProtocolRegistry)),
      in_memory_(in_memory),
      ssl_config_(network::mojom::SSLConfig::New()) {
  user_agent_ = ElectronBrowserClient::Get()->GetUserAgent();

  // Read options.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  use_cache_ = !command_line->HasSwitch(switches::kDisableHttpCache);
  options.GetBoolean("cache", &use_cache_);

  base::StringToInt(command_line->GetSwitchValueASCII(switches::kDiskCacheSize),
                    &max_cache_size_);

  base::PathService::Get(DIR_BROWSER_DATA, &path_);
  if (!in_memory && !partition.empty())
    path_ = path_.Append(FILE_PATH_LITERAL("Partitions"))
                .Append(base::FilePath::FromUTF8Unsafe(
                    MakePartitionName(partition)));

  BrowserContextDependencyManager::GetInstance()->MarkBrowserContextLive(this);

  // Initialize Pref Registry.
  InitPrefs();

  cookie_change_notifier_ = std::make_unique<CookieChangeNotifier>(this);

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  if (!in_memory_) {
    BrowserContextDependencyManager::GetInstance()
        ->CreateBrowserContextServices(this);

    extension_system_ = static_cast<extensions::ElectronExtensionSystem*>(
        extensions::ExtensionSystem::Get(this));
    extension_system_->InitForRegularProfile(true /* extensions_enabled */);
    extension_system_->FinishInitialization();
  }
#endif
}

ElectronBrowserContext::~ElectronBrowserContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  NotifyWillBeDestroyed();
  // Notify any keyed services of browser context destruction.
  BrowserContextDependencyManager::GetInstance()->DestroyBrowserContextServices(
      this);
  ShutdownStoragePartitions();

  BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE,
                            std::move(resource_context_));
}

void ElectronBrowserContext::InitPrefs() {
  auto prefs_path = GetPath().Append(FILE_PATH_LITERAL("Preferences"));
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  PrefServiceFactory prefs_factory;
  scoped_refptr<JsonPrefStore> pref_store =
      base::MakeRefCounted<JsonPrefStore>(prefs_path);
  pref_store->ReadPrefs();  // Synchronous.
  prefs_factory.set_user_prefs(pref_store);

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  if (!in_memory_) {
    auto* ext_pref_store = new ExtensionPrefStore(
        ExtensionPrefValueMapFactory::GetForBrowserContext(this),
        IsOffTheRecord());
    prefs_factory.set_extension_prefs(ext_pref_store);
  }
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS) || \
    BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  auto registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();
#else
  auto registry = base::MakeRefCounted<PrefRegistrySimple>();
#endif

  registry->RegisterFilePathPref(prefs::kSelectFileLastDirectory,
                                 base::FilePath());
  base::FilePath download_dir;
  base::PathService::Get(chrome::DIR_DEFAULT_DOWNLOADS, &download_dir);
  registry->RegisterFilePathPref(prefs::kDownloadDefaultDirectory,
                                 download_dir);
  registry->RegisterDictionaryPref(prefs::kDevToolsFileSystemPaths);
  InspectableWebContents::RegisterPrefs(registry.get());
  MediaDeviceIDSalt::RegisterPrefs(registry.get());
  ZoomLevelDelegate::RegisterPrefs(registry.get());
  PrefProxyConfigTrackerImpl::RegisterPrefs(registry.get());
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  if (!in_memory_)
    extensions::ExtensionPrefs::RegisterProfilePrefs(registry.get());
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  BrowserContextDependencyManager::GetInstance()
      ->RegisterProfilePrefsForServices(registry.get());

  language::LanguagePrefs::RegisterProfilePrefs(registry.get());
#endif

  prefs_ = prefs_factory.Create(
      registry.get(),
      std::make_unique<PrefStoreDelegate>(weak_factory_.GetWeakPtr()));
  prefs_->UpdateCommandLinePrefStore(new ValueMapPrefStore);
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS) || \
    BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  user_prefs::UserPrefs::Set(this, prefs_.get());
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  auto* current_dictionaries =
      prefs()->Get(spellcheck::prefs::kSpellCheckDictionaries);
  // No configured dictionaries, the default will be en-US
  if (current_dictionaries->GetList().empty()) {
    std::string default_code = spellcheck::GetCorrespondingSpellCheckLanguage(
        base::i18n::GetConfiguredLocale());
    if (!default_code.empty()) {
      base::ListValue language_codes;
      language_codes.Append(default_code);
      prefs()->Set(spellcheck::prefs::kSpellCheckDictionaries, language_codes);
    }
  }
#endif
}

void ElectronBrowserContext::SetUserAgent(const std::string& user_agent) {
  user_agent_ = user_agent;
}

base::FilePath ElectronBrowserContext::GetPath() {
  return path_;
}

bool ElectronBrowserContext::IsOffTheRecord() {
  return in_memory_;
}

bool ElectronBrowserContext::CanUseHttpCache() const {
  return use_cache_;
}

int ElectronBrowserContext::GetMaxCacheSize() const {
  return max_cache_size_;
}

content::ResourceContext* ElectronBrowserContext::GetResourceContext() {
  if (!resource_context_)
    resource_context_ = std::make_unique<content::ResourceContext>();
  return resource_context_.get();
}

std::string ElectronBrowserContext::GetMediaDeviceIDSalt() {
  if (!media_device_id_salt_.get())
    media_device_id_salt_ = std::make_unique<MediaDeviceIDSalt>(prefs_.get());
  return media_device_id_salt_->GetSalt();
}

std::unique_ptr<content::ZoomLevelDelegate>
ElectronBrowserContext::CreateZoomLevelDelegate(
    const base::FilePath& partition_path) {
  if (!IsOffTheRecord()) {
    return std::make_unique<ZoomLevelDelegate>(prefs(), partition_path);
  }
  return std::unique_ptr<content::ZoomLevelDelegate>();
}

content::DownloadManagerDelegate*
ElectronBrowserContext::GetDownloadManagerDelegate() {
  if (!download_manager_delegate_.get()) {
    auto* download_manager = this->GetDownloadManager();
    download_manager_delegate_ =
        std::make_unique<ElectronDownloadManagerDelegate>(download_manager);
  }
  return download_manager_delegate_.get();
}

content::BrowserPluginGuestManager* ElectronBrowserContext::GetGuestManager() {
  if (!guest_manager_)
    guest_manager_ = std::make_unique<WebViewManager>();
  return guest_manager_.get();
}

content::PlatformNotificationService*
ElectronBrowserContext::GetPlatformNotificationService() {
  return ElectronBrowserClient::Get()->GetPlatformNotificationService();
}

content::PermissionControllerDelegate*
ElectronBrowserContext::GetPermissionControllerDelegate() {
  if (!permission_manager_.get())
    permission_manager_ = std::make_unique<ElectronPermissionManager>();
  return permission_manager_.get();
}

storage::SpecialStoragePolicy*
ElectronBrowserContext::GetSpecialStoragePolicy() {
  return storage_policy_.get();
}

std::string ElectronBrowserContext::GetUserAgent() const {
  return user_agent_;
}

predictors::PreconnectManager* ElectronBrowserContext::GetPreconnectManager() {
  if (!preconnect_manager_.get()) {
    preconnect_manager_ =
        std::make_unique<predictors::PreconnectManager>(nullptr, this);
  }
  return preconnect_manager_.get();
}

scoped_refptr<network::SharedURLLoaderFactory>
ElectronBrowserContext::GetURLLoaderFactory() {
  if (url_loader_factory_)
    return url_loader_factory_;

  mojo::PendingRemote<network::mojom::URLLoaderFactory> network_factory_remote;
  mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver =
      network_factory_remote.InitWithNewPipeAndPassReceiver();

  // Consult the embedder.
  mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>
      header_client;
  static_cast<content::ContentBrowserClient*>(ElectronBrowserClient::Get())
      ->WillCreateURLLoaderFactory(
          this, nullptr, -1,
          content::ContentBrowserClient::URLLoaderFactoryType::kNavigation,
          url::Origin(), absl::nullopt, ukm::kInvalidSourceIdObj,
          &factory_receiver, &header_client, nullptr, nullptr, nullptr);

  network::mojom::URLLoaderFactoryParamsPtr params =
      network::mojom::URLLoaderFactoryParams::New();
  params->header_client = std::move(header_client);
  params->process_id = network::mojom::kBrowserProcessId;
  params->is_trusted = true;
  params->is_corb_enabled = false;
  // The tests of net module would fail if this setting is true, it seems that
  // the non-NetworkService implementation always has web security enabled.
  params->disable_web_security = false;

  auto* storage_partition = GetDefaultStoragePartition();
  params->url_loader_network_observer =
      storage_partition->CreateURLLoaderNetworkObserverForNavigationRequest(-1);
  storage_partition->GetNetworkContext()->CreateURLLoaderFactory(
      std::move(factory_receiver), std::move(params));
  url_loader_factory_ =
      base::MakeRefCounted<network::WrapperSharedURLLoaderFactory>(
          std::move(network_factory_remote));
  return url_loader_factory_;
}

content::PushMessagingService*
ElectronBrowserContext::GetPushMessagingService() {
  return nullptr;
}

content::SSLHostStateDelegate*
ElectronBrowserContext::GetSSLHostStateDelegate() {
  return nullptr;
}

content::BackgroundFetchDelegate*
ElectronBrowserContext::GetBackgroundFetchDelegate() {
  return nullptr;
}

content::BackgroundSyncController*
ElectronBrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

content::BrowsingDataRemoverDelegate*
ElectronBrowserContext::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

content::ClientHintsControllerDelegate*
ElectronBrowserContext::GetClientHintsControllerDelegate() {
  return nullptr;
}

content::StorageNotificationService*
ElectronBrowserContext::GetStorageNotificationService() {
  return nullptr;
}

ResolveProxyHelper* ElectronBrowserContext::GetResolveProxyHelper() {
  if (!resolve_proxy_helper_) {
    resolve_proxy_helper_ = base::MakeRefCounted<ResolveProxyHelper>(this);
  }
  return resolve_proxy_helper_.get();
}

network::mojom::SSLConfigPtr ElectronBrowserContext::GetSSLConfig() {
  return ssl_config_.Clone();
}

void ElectronBrowserContext::SetSSLConfig(network::mojom::SSLConfigPtr config) {
  ssl_config_ = std::move(config);
  if (ssl_config_client_) {
    ssl_config_client_->OnSSLConfigUpdated(ssl_config_.Clone());
  }
}

void ElectronBrowserContext::SetSSLConfigClient(
    mojo::Remote<network::mojom::SSLConfigClient> client) {
  ssl_config_client_ = std::move(client);
}

// static
ElectronBrowserContext* ElectronBrowserContext::From(
    const std::string& partition,
    bool in_memory,
    base::DictionaryValue options) {
  PartitionKey key(partition, in_memory);
  ElectronBrowserContext* browser_context = browser_context_map()[key].get();
  if (browser_context) {
    return browser_context;
  }

  auto* new_context =
      new ElectronBrowserContext(partition, in_memory, std::move(options));
  browser_context_map()[key] =
      std::unique_ptr<ElectronBrowserContext>(new_context);
  return new_context;
}

}  // namespace electron
