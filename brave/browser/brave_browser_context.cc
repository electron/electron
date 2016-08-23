// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/browser/brave_browser_context.h"

#include "atom/browser/net/atom_url_request_job_factory.h"
#include "base/path_service.h"
#include "brave/browser/brave_permission_manager.h"
#include "brightray/browser/brightray_paths.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/common/pref_names.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/browser/webdata/autofill_table.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_filter.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/syncable_prefs/pref_service_syncable.h"
#include "components/syncable_prefs/pref_service_syncable_factory.h"
#include "components/user_prefs/user_prefs.h"
#include "components/ui/zoom/zoom_event_manager.h"
#include "components/webdata/common/webdata_constants.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/cookies/cookie_store.h"
#include "net/url_request/url_request_context.h"

#if defined(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_browser_client_extensions_part.h"
#include "atom/browser/extensions/atom_extension_system_factory.h"
#include "atom/browser/extensions/atom_extensions_network_delegate.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "extensions/browser/extension_pref_store.h"
#include "extensions/browser/extension_pref_value_map_factory.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_protocols.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extensions_browser_client.h"
#endif

using content::BrowserThread;
using content::HostZoomMap;

#if defined(ENABLE_EXTENSIONS)
namespace {

void NotifyOTRProfileCreatedOnIOThread(void* original_profile,
                                       void* otr_profile) {
  extensions::ExtensionWebRequestEventRouter::GetInstance()
      ->OnOTRBrowserContextCreated(original_profile, otr_profile);
}

void NotifyOTRProfileDestroyedOnIOThread(void* original_profile,
                                         void* otr_profile) {
  extensions::ExtensionWebRequestEventRouter::GetInstance()
      ->OnOTRBrowserContextDestroyed(original_profile, otr_profile);
}

}  // namespace
#endif

namespace brave {

void DatabaseErrorCallback(sql::InitStatus status) {
  // TODO(bridiver) - we should send a notification of failure
  LOG(WARNING) << "initializing autocomplete database failed";
}

BraveBrowserContext::BraveBrowserContext(const std::string& partition,
                                       bool in_memory,
                                       const base::DictionaryValue& options)
    : atom::AtomBrowserContext(partition, in_memory, options),
      pref_registry_(new user_prefs::PrefRegistrySyncable),
#if defined(ENABLE_EXTENSIONS)
      network_delegate_(new extensions::AtomExtensionsNetworkDelegate(this)),
#else
      network_delegate_(new AtomNetworkDelegate),
#endif
      partition_(partition) {
  if (in_memory) {
    original_context_ = static_cast<BraveBrowserContext*>(
        atom::AtomBrowserContext::From(partition, false).get());
    original_context_->otr_context_ = this;
  }
  InitPrefs();

  if (in_memory) {
    TrackZoomLevelsFromParent();
#if defined(ENABLE_EXTENSIONS)
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&NotifyOTRProfileCreatedOnIOThread,
          base::Unretained(original_context_),
          base::Unretained(this)));
#endif
  } else {
    // Initialize autofill db
    base::FilePath user_dir;
    PathService::Get(brightray::DIR_USER_DATA, &user_dir);
    user_dir = user_dir.Append(kWebDataFilename);
    user_dir = user_dir.AddExtension(
        base::FilePath::StringType(partition.begin(), partition.end()));

    CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    web_database_ = new WebDatabaseService(user_dir,
        BrowserThread::GetMessageLoopProxyForThread(BrowserThread::UI),
        BrowserThread::GetMessageLoopProxyForThread(BrowserThread::DB));
    web_database_->AddTable(base::WrapUnique(new autofill::AutofillTable));
    web_database_->LoadDatabase();

    autofill_data_ = new autofill::AutofillWebDataService(
        web_database_,
        BrowserThread::GetMessageLoopProxyForThread(BrowserThread::UI),
        BrowserThread::GetMessageLoopProxyForThread(BrowserThread::DB),
        base::Bind(&DatabaseErrorCallback));
    autofill_data_->Init();
  }
}

BraveBrowserContext::~BraveBrowserContext() {
  NotifyWillBeDestroyed(this);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PROFILE_DESTROYED,
      content::Source<BraveBrowserContext>(this),
      content::NotificationService::NoDetails());

  if (user_prefs_registrar_.get())
    user_prefs_registrar_->RemoveAll();

  if (!IsOffTheRecord()) {
    // temporary fix for https://github.com/brave/browser-laptop/issues/2335
    // TODO(brivdiver) - it seems like something is holding onto a reference to
    // url_request_context_getter or the url_request_context and is preventing
    // it from being destroyed
    url_request_context_getter()->GetURLRequestContext()->cookie_store()->
        FlushStore(base::Closure());

    bool prefs_loaded = user_prefs_->GetInitializationStatus() !=
        PrefService::INITIALIZATION_STATUS_WAITING;

    if (prefs_loaded) {
      user_prefs_->CommitPendingWrite();
    }
  } else {
#if defined(ENABLE_EXTENSIONS)
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&NotifyOTRProfileDestroyedOnIOThread, base::Unretained(original_context_), base::Unretained(this)));
#endif
  }

  if (otr_context_.get()) {
    auto user_prefs = user_prefs::UserPrefs::Get(otr_context_.get());
    if (user_prefs)
      user_prefs->ClearMutableValues();
    otr_context_ = NULL;
#if defined(ENABLE_EXTENSIONS)
    ExtensionPrefValueMapFactory::GetForBrowserContext(this)->
        ClearAllIncognitoSessionOnlyPreferences();
#endif
  }

  BrowserContextDependencyManager::GetInstance()->
      DestroyBrowserContextServices(this);

  autofill_data_->ShutdownOnUIThread();
  web_database_->ShutdownDatabase();
}

// static
BraveBrowserContext* BraveBrowserContext::FromBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<BraveBrowserContext*>(browser_context);
}

BraveBrowserContext* BraveBrowserContext::original_context() {
  if (!IsOffTheRecord()) {
    return this;
  }
  return original_context_;
}

BraveBrowserContext* BraveBrowserContext::otr_context() {
  if (IsOffTheRecord()) {
    return this;
  }

  if (otr_context_.get()) {
    return otr_context_.get();
  }

  return nullptr;
}

void BraveBrowserContext::TrackZoomLevelsFromParent() {
  DCHECK_NE(true, IsOffTheRecord());

  // Here we only want to use zoom levels stored in the main-context's default
  // storage partition. We're not interested in zoom levels in special
  // partitions, e.g. those used by WebViewGuests.
  HostZoomMap* host_zoom_map = HostZoomMap::GetDefaultForBrowserContext(this);
  HostZoomMap* parent_host_zoom_map =
      HostZoomMap::GetDefaultForBrowserContext(original_context_);
  host_zoom_map->CopyFrom(parent_host_zoom_map);
  // Observe parent profile's HostZoomMap changes so they can also be applied
  // to this profile's HostZoomMap.
  track_zoom_subscription_ = parent_host_zoom_map->AddZoomLevelChangedCallback(
      base::Bind(&BraveBrowserContext::OnParentZoomLevelChanged,
                 base::Unretained(this)));
  if (!original_context_->GetZoomLevelPrefs())
    return;

  // Also track changes to the parent profile's default zoom level.
  parent_default_zoom_level_subscription_ =
      original_context_->GetZoomLevelPrefs()->RegisterDefaultZoomLevelCallback(
          base::Bind(&BraveBrowserContext::UpdateDefaultZoomLevel,
                     base::Unretained(this)));
}

ChromeZoomLevelPrefs* BraveBrowserContext::GetZoomLevelPrefs() {
  return static_cast<ChromeZoomLevelPrefs*>(
      GetDefaultStoragePartition(this)->GetZoomLevelDelegate());
}

void BraveBrowserContext::OnParentZoomLevelChanged(
    const HostZoomMap::ZoomLevelChange& change) {
  HostZoomMap* host_zoom_map = HostZoomMap::GetDefaultForBrowserContext(this);
  switch (change.mode) {
    case HostZoomMap::ZOOM_CHANGED_TEMPORARY_ZOOM:
      return;
    case HostZoomMap::ZOOM_CHANGED_FOR_HOST:
      host_zoom_map->SetZoomLevelForHost(change.host, change.zoom_level);
      return;
    case HostZoomMap::ZOOM_CHANGED_FOR_SCHEME_AND_HOST:
      host_zoom_map->SetZoomLevelForHostAndScheme(change.scheme,
          change.host,
          change.zoom_level);
      return;
    case HostZoomMap::PAGE_SCALE_IS_ONE_CHANGED:
      return;
  }
}

void BraveBrowserContext::UpdateDefaultZoomLevel() {
  HostZoomMap* host_zoom_map = HostZoomMap::GetDefaultForBrowserContext(this);
  double default_zoom_level =
      original_context_->GetZoomLevelPrefs()->GetDefaultZoomLevelPref();
  host_zoom_map->SetDefaultZoomLevel(default_zoom_level);
  // HostZoomMap does not trigger zoom notification events when the default
  // zoom level is set, so we need to do it here.
  ui_zoom::ZoomEventManager::GetForBrowserContext(this)
      ->OnDefaultZoomLevelChanged();
}

content::PermissionManager* BraveBrowserContext::GetPermissionManager() {
  if (!permission_manager_.get())
    permission_manager_.reset(new BravePermissionManager);
  return permission_manager_.get();
}

net::NetworkDelegate* BraveBrowserContext::CreateNetworkDelegate() {
  return network_delegate_;
}

std::unique_ptr<net::URLRequestJobFactory>
BraveBrowserContext::CreateURLRequestJobFactory(
    content::ProtocolHandlerMap* protocol_handlers) {
  std::unique_ptr<net::URLRequestJobFactory> job_factory =
      AtomBrowserContext::CreateURLRequestJobFactory(protocol_handlers);
#if defined(ENABLE_EXTENSIONS)
  extensions::InfoMap* extension_info_map =
      extensions::AtomExtensionSystemFactory::GetInstance()->
        GetForBrowserContext(this)->info_map();
  static_cast<atom::AtomURLRequestJobFactory*>(job_factory.get())->SetProtocolHandler(
      extensions::kExtensionScheme,
      extensions::CreateExtensionProtocolHandler(IsOffTheRecord(),
                                                 extension_info_map));
#endif

  std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
    protocol_handler_interceptor =
        ProtocolHandlerRegistryFactory::GetForBrowserContext(this)
          ->CreateJobInterceptorFactory();

  protocol_handler_interceptor->Chain(std::move(job_factory));
  return std::move(protocol_handler_interceptor);
}

void BraveBrowserContext::RegisterPrefs(PrefRegistrySimple* pref_registry) {
  AtomBrowserContext::RegisterPrefs(pref_registry);
#if defined(ENABLE_EXTENSIONS)
  PrefStore* extension_prefs = new ExtensionPrefStore(
      ExtensionPrefValueMapFactory::GetForBrowserContext(original_context()),
      IsOffTheRecord());
#else
  PrefStore* extension_prefs = NULL;
#endif
  user_prefs_registrar_.reset(new PrefChangeRegistrar());

  bool async = false;

  if (IsOffTheRecord()) {
    user_prefs_.reset(
        original_context()->user_prefs()->CreateIncognitoPrefService(
          extension_prefs, overlay_pref_names_));
    user_prefs::UserPrefs::Set(this, user_prefs_.get());
  } else {
    pref_registry_->RegisterDictionaryPref(prefs::kPartitionDefaultZoomLevel);
    pref_registry_->RegisterDictionaryPref(prefs::kPartitionPerHostZoomLevels);
    // TODO(bridiver) - is this necessary or is it covered by
    // BrowserContextDependencyManager
    ProtocolHandlerRegistry::RegisterProfilePrefs(pref_registry_.get());
    autofill::AutofillManager::RegisterProfilePrefs(pref_registry_.get());
#if defined(ENABLE_EXTENSIONS)
    extensions::AtomBrowserClientExtensionsPart::RegisterProfilePrefs(
        pref_registry_.get());
    extensions::ExtensionPrefs::RegisterProfilePrefs(pref_registry_.get());
#endif
    BrowserContextDependencyManager::GetInstance()->
        RegisterProfilePrefsForServices(this, pref_registry_.get());

    // create profile prefs
    base::FilePath filepath = GetPath().Append(
        FILE_PATH_LITERAL("UserPrefs"));
    scoped_refptr<base::SequencedTaskRunner> task_runner =
        JsonPrefStore::GetTaskRunnerForFile(
            filepath, BrowserThread::GetBlockingPool());
    scoped_refptr<JsonPrefStore> pref_store =
        new JsonPrefStore(filepath, task_runner, std::unique_ptr<PrefFilter>());

    // prepare factory
    syncable_prefs::PrefServiceSyncableFactory factory;
    factory.set_async(async);
    factory.set_extension_prefs(extension_prefs);
    factory.set_user_prefs(pref_store);
    user_prefs_ = factory.CreateSyncable(pref_registry_.get());
    user_prefs::UserPrefs::Set(this, user_prefs_.get());
  }

  if (async) {
    user_prefs_->AddPrefInitObserver(base::Bind(
        &BraveBrowserContext::OnPrefsLoaded, base::Unretained(this)));
  } else {
    OnPrefsLoaded(true);
  }
}

void BraveBrowserContext::OnPrefsLoaded(bool success) {
  CHECK(success);

  BrowserContextDependencyManager::GetInstance()->
      CreateBrowserContextServices(this);

  if (!IsOffTheRecord()) {
#if defined(ENABLE_EXTENSIONS)
    extensions::ExtensionSystem::Get(this)->InitForRegularProfile(true);
    static_cast<extensions::AtomExtensionsNetworkDelegate*>(network_delegate_)->
        set_extension_info_map(
            extensions::ExtensionSystem::Get(this)->info_map());
#endif
    content::BrowserContext::GetDefaultStoragePartition(this)->
        GetDOMStorageContext()->SetSaveSessionStorageOnDisk();
  }

  user_prefs_registrar_->Init(user_prefs_.get());
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PROFILE_CREATED,
      content::Source<BraveBrowserContext>(this),
      content::NotificationService::NoDetails());
}

content::ResourceContext* BraveBrowserContext::GetResourceContext() {
  content::BrowserContext::EnsureResourceContextInitialized(this);
  return brightray::BrowserContext::GetResourceContext();
}

std::unique_ptr<content::ZoomLevelDelegate>
BraveBrowserContext::CreateZoomLevelDelegate(const base::FilePath& partition_path) {
  return base::WrapUnique(new ChromeZoomLevelPrefs(
      GetPrefs(), GetPath(), partition_path,
      ui_zoom::ZoomEventManager::GetForBrowserContext(this)->GetWeakPtr()));
}

scoped_refptr<autofill::AutofillWebDataService>
BraveBrowserContext::GetAutofillWebdataService() {
  return original_context()->autofill_data_;
}

}  // namespace brave

namespace atom {

// TODO(bridiver) find a better way to do this
// static
scoped_refptr<AtomBrowserContext> AtomBrowserContext::From(
    const std::string& partition, bool in_memory,
    const base::DictionaryValue& options) {
  auto browser_context = brightray::BrowserContext::Get(partition, in_memory);
  if (browser_context)
    return static_cast<AtomBrowserContext*>(browser_context.get());

  return new brave::BraveBrowserContext(partition, in_memory, options);
}

}  // namespace brightray

