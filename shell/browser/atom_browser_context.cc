// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_browser_context.h"

#include <memory>

#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
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
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/escape.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "shell/browser/api/atom_api_url_loader.h"
#include "shell/browser/atom_browser_client.h"
#include "shell/browser/atom_browser_main_parts.h"
#include "shell/browser/atom_download_manager_delegate.h"
#include "shell/browser/atom_paths.h"
#include "shell/browser/atom_permission_manager.h"
#include "shell/browser/cookie_change_notifier.h"
#include "shell/browser/net/resolve_proxy_helper.h"
#include "shell/browser/pref_store_delegate.h"
#include "shell/browser/special_storage_policy.h"
#include "shell/browser/ui/inspectable_web_contents_impl.h"
#include "shell/browser/web_view_manager.h"
#include "shell/browser/zoom_level_delegate.h"
#include "shell/common/application_info.h"
#include "shell/common/options_switches.h"

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/browser/browser_context_keyed_service_factories.h"
#include "extensions/browser/extension_pref_store.h"
#include "extensions/browser/extension_pref_value_map_factory.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/extension_api.h"
#include "shell/browser/extensions/atom_browser_context_keyed_service_factories.h"
#include "shell/browser/extensions/atom_extension_system.h"
#include "shell/browser/extensions/atom_extension_system_factory.h"
#include "shell/browser/extensions/atom_extensions_browser_client.h"
#include "shell/common/extensions/atom_extensions_client.h"
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
AtomBrowserContext::BrowserContextMap AtomBrowserContext::browser_context_map_;

AtomBrowserContext::AtomBrowserContext(const std::string& partition,
                                       bool in_memory,
                                       base::DictionaryValue options)
    : base::RefCountedDeleteOnSequence<AtomBrowserContext>(
          base::ThreadTaskRunnerHandle::Get()),
      in_memory_pref_store_(nullptr),
      storage_policy_(new SpecialStoragePolicy),
      in_memory_(in_memory),
      weak_factory_(this) {
  user_agent_ = AtomBrowserClient::Get()->GetUserAgent();

  // Read options.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  use_cache_ = !command_line->HasSwitch(switches::kDisableHttpCache);
  options.GetBoolean("cache", &use_cache_);

  base::StringToInt(command_line->GetSwitchValueASCII(switches::kDiskCacheSize),
                    &max_cache_size_);

  if (!base::PathService::Get(DIR_USER_DATA, &path_)) {
    base::PathService::Get(DIR_APP_DATA, &path_);
    path_ = path_.Append(base::FilePath::FromUTF8Unsafe(GetApplicationName()));
    base::PathService::Override(DIR_USER_DATA, path_);
    base::PathService::Override(chrome::DIR_USER_DATA, path_);
  }

  if (!in_memory && !partition.empty())
    path_ = path_.Append(FILE_PATH_LITERAL("Partitions"))
                .Append(base::FilePath::FromUTF8Unsafe(
                    MakePartitionName(partition)));

  content::BrowserContext::Initialize(this, path_);

  BrowserContextDependencyManager::GetInstance()->MarkBrowserContextLive(this);

  // Initialize Pref Registry.
  InitPrefs();

  cookie_change_notifier_ = std::make_unique<CookieChangeNotifier>(this);

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  BrowserContextDependencyManager::GetInstance()->CreateBrowserContextServices(
      this);

  extension_system_ = static_cast<extensions::AtomExtensionSystem*>(
      extensions::ExtensionSystem::Get(this));
  extension_system_->InitForRegularProfile(true /* extensions_enabled */);
  extension_system_->FinishInitialization();
#endif
}

AtomBrowserContext::~AtomBrowserContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  NotifyWillBeDestroyed(this);
  // Notify any keyed services of browser context destruction.
  BrowserContextDependencyManager::GetInstance()->DestroyBrowserContextServices(
      this);
  ShutdownStoragePartitions();

  BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE,
                            std::move(resource_context_));
}

void AtomBrowserContext::InitPrefs() {
  auto prefs_path = GetPath().Append(FILE_PATH_LITERAL("Preferences"));
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  PrefServiceFactory prefs_factory;
  scoped_refptr<JsonPrefStore> pref_store =
      base::MakeRefCounted<JsonPrefStore>(prefs_path);
  pref_store->ReadPrefs();  // Synchronous.
  prefs_factory.set_user_prefs(pref_store);

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  auto* ext_pref_store = new ExtensionPrefStore(
      ExtensionPrefValueMapFactory::GetForBrowserContext(this),
      IsOffTheRecord());
  prefs_factory.set_extension_prefs(ext_pref_store);
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS) || \
    BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  auto registry = WrapRefCounted(new user_prefs::PrefRegistrySyncable);
#else
  auto registry = WrapRefCounted(new PrefRegistrySimple);
#endif

  registry->RegisterFilePathPref(prefs::kSelectFileLastDirectory,
                                 base::FilePath());
  base::FilePath download_dir;
  base::PathService::Get(chrome::DIR_DEFAULT_DOWNLOADS, &download_dir);
  registry->RegisterFilePathPref(prefs::kDownloadDefaultDirectory,
                                 download_dir);
  registry->RegisterDictionaryPref(prefs::kDevToolsFileSystemPaths);
  InspectableWebContentsImpl::RegisterPrefs(registry.get());
  MediaDeviceIDSalt::RegisterPrefs(registry.get());
  ZoomLevelDelegate::RegisterPrefs(registry.get());
  PrefProxyConfigTrackerImpl::RegisterPrefs(registry.get());
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
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
  if (current_dictionaries->GetList().size() == 0) {
    std::string default_code = spellcheck::GetCorrespondingSpellCheckLanguage(
        base::i18n::GetConfiguredLocale());
    if (!default_code.empty()) {
      base::ListValue language_codes;
      language_codes.AppendString(default_code);
      prefs()->Set(spellcheck::prefs::kSpellCheckDictionaries, language_codes);
    }
  }
#endif
}

void AtomBrowserContext::SetUserAgent(const std::string& user_agent) {
  user_agent_ = user_agent;
}

base::FilePath AtomBrowserContext::GetPath() {
  return path_;
}

bool AtomBrowserContext::IsOffTheRecord() {
  return in_memory_;
}

bool AtomBrowserContext::CanUseHttpCache() const {
  return use_cache_;
}

int AtomBrowserContext::GetMaxCacheSize() const {
  return max_cache_size_;
}

content::ResourceContext* AtomBrowserContext::GetResourceContext() {
  if (!resource_context_)
    resource_context_ = std::make_unique<content::ResourceContext>();
  return resource_context_.get();
}

std::string AtomBrowserContext::GetMediaDeviceIDSalt() {
  if (!media_device_id_salt_.get())
    media_device_id_salt_ = std::make_unique<MediaDeviceIDSalt>(prefs_.get());
  return media_device_id_salt_->GetSalt();
}

std::unique_ptr<content::ZoomLevelDelegate>
AtomBrowserContext::CreateZoomLevelDelegate(
    const base::FilePath& partition_path) {
  if (!IsOffTheRecord()) {
    return std::make_unique<ZoomLevelDelegate>(prefs(), partition_path);
  }
  return std::unique_ptr<content::ZoomLevelDelegate>();
}

content::DownloadManagerDelegate*
AtomBrowserContext::GetDownloadManagerDelegate() {
  if (!download_manager_delegate_.get()) {
    auto* download_manager = content::BrowserContext::GetDownloadManager(this);
    download_manager_delegate_ =
        std::make_unique<AtomDownloadManagerDelegate>(download_manager);
  }
  return download_manager_delegate_.get();
}

content::BrowserPluginGuestManager* AtomBrowserContext::GetGuestManager() {
  if (!guest_manager_)
    guest_manager_ = std::make_unique<WebViewManager>();
  return guest_manager_.get();
}

content::PermissionControllerDelegate*
AtomBrowserContext::GetPermissionControllerDelegate() {
  if (!permission_manager_.get())
    permission_manager_ = std::make_unique<AtomPermissionManager>();
  return permission_manager_.get();
}

storage::SpecialStoragePolicy* AtomBrowserContext::GetSpecialStoragePolicy() {
  return storage_policy_.get();
}

std::string AtomBrowserContext::GetUserAgent() const {
  return user_agent_;
}

predictors::PreconnectManager* AtomBrowserContext::GetPreconnectManager() {
  if (!preconnect_manager_.get()) {
    preconnect_manager_ =
        std::make_unique<predictors::PreconnectManager>(nullptr, this);
  }
  return preconnect_manager_.get();
}

scoped_refptr<network::SharedURLLoaderFactory>
AtomBrowserContext::GetURLLoaderFactory() {
  if (url_loader_factory_)
    return url_loader_factory_;

  mojo::PendingRemote<network::mojom::URLLoaderFactory> network_factory_remote;
  mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver =
      network_factory_remote.InitWithNewPipeAndPassReceiver();

  // Consult the embedder.
  mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>
      header_client;
  static_cast<content::ContentBrowserClient*>(AtomBrowserClient::Get())
      ->WillCreateURLLoaderFactory(
          this, nullptr, -1,
          content::ContentBrowserClient::URLLoaderFactoryType::kNavigation,
          url::Origin(), base::nullopt, &factory_receiver, &header_client,
          nullptr, nullptr, nullptr);

  network::mojom::URLLoaderFactoryParamsPtr params =
      network::mojom::URLLoaderFactoryParams::New();
  params->header_client = std::move(header_client);
  params->auth_client = auth_client_.BindNewPipeAndPassRemote();
  params->process_id = network::mojom::kBrowserProcessId;
  params->is_trusted = true;
  params->is_corb_enabled = false;
  // The tests of net module would fail if this setting is true, it seems that
  // the non-NetworkService implementation always has web security enabled.
  params->disable_web_security = false;

  auto* storage_partition =
      content::BrowserContext::GetDefaultStoragePartition(this);
  storage_partition->GetNetworkContext()->CreateURLLoaderFactory(
      std::move(factory_receiver), std::move(params));
  url_loader_factory_ =
      base::MakeRefCounted<network::WrapperSharedURLLoaderFactory>(
          std::move(network_factory_remote));
  return url_loader_factory_;
}

class AuthResponder : public network::mojom::TrustedAuthClient {
 public:
  AuthResponder() {}
  ~AuthResponder() override = default;

 private:
  void OnAuthRequired(
      const base::Optional<::base::UnguessableToken>& window_id,
      uint32_t process_id,
      uint32_t routing_id,
      uint32_t request_id,
      const ::GURL& url,
      bool first_auth_attempt,
      const ::net::AuthChallengeInfo& auth_info,
      ::network::mojom::URLResponseHeadPtr head,
      mojo::PendingRemote<network::mojom::AuthChallengeResponder>
          auth_challenge_responder) override {
    api::SimpleURLLoaderWrapper* url_loader =
        api::SimpleURLLoaderWrapper::FromID(routing_id);
    if (url_loader) {
      url_loader->OnAuthRequired(url, first_auth_attempt, auth_info,
                                 std::move(head),
                                 std::move(auth_challenge_responder));
    }
  }
};

void AtomBrowserContext::OnLoaderCreated(
    int32_t request_id,
    mojo::PendingReceiver<network::mojom::TrustedAuthClient> auth_client) {
  mojo::MakeSelfOwnedReceiver(std::make_unique<AuthResponder>(),
                              std::move(auth_client));
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

content::ClientHintsControllerDelegate*
AtomBrowserContext::GetClientHintsControllerDelegate() {
  return nullptr;
}

content::StorageNotificationService*
AtomBrowserContext::GetStorageNotificationService() {
  return nullptr;
}

void AtomBrowserContext::SetCorsOriginAccessListForOrigin(
    const url::Origin& source_origin,
    std::vector<network::mojom::CorsOriginPatternPtr> allow_patterns,
    std::vector<network::mojom::CorsOriginPatternPtr> block_patterns,
    base::OnceClosure closure) {
  // TODO(nornagon): actually set the CORS access lists. This is called from
  // extensions/browser/renderer_startup_helper.cc.
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, std::move(closure));
}

ResolveProxyHelper* AtomBrowserContext::GetResolveProxyHelper() {
  if (!resolve_proxy_helper_) {
    resolve_proxy_helper_ = base::MakeRefCounted<ResolveProxyHelper>(this);
  }
  return resolve_proxy_helper_.get();
}

// static
scoped_refptr<AtomBrowserContext> AtomBrowserContext::From(
    const std::string& partition,
    bool in_memory,
    base::DictionaryValue options) {
  PartitionKey key(partition, in_memory);
  auto* browser_context = browser_context_map_[key].get();
  if (browser_context)
    return scoped_refptr<AtomBrowserContext>(browser_context);

  auto* new_context =
      new AtomBrowserContext(partition, in_memory, std::move(options));
  browser_context_map_[key] = new_context->GetWeakPtr();
  return scoped_refptr<AtomBrowserContext>(new_context);
}

}  // namespace electron
