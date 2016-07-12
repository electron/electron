// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/browser/brave_browser_context.h"

#include "atom/browser/net/atom_url_request_job_factory.h"
#include "brave/browser/brave_permission_manager.h"
#include "chrome/browser/chrome_notification_types.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_filter.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/syncable_prefs/pref_service_syncable.h"
#include "components/syncable_prefs/pref_service_syncable_factory.h"
#include "components/user_prefs/user_prefs.h"
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
#include "extensions/browser/extension_pref_store.h"
#include "extensions/browser/extension_pref_value_map_factory.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_protocols.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extensions_browser_client.h"
#endif

using content::BrowserThread;

namespace brave {

BraveBrowserContext::BraveBrowserContext(const std::string& partition,
                                       bool in_memory)
    : atom::AtomBrowserContext(partition, in_memory),
      pref_registry_(new user_prefs::PrefRegistrySyncable),
#if defined(ENABLE_EXTENSIONS)
      network_delegate_(new extensions::AtomExtensionsNetworkDelegate(this)),
#else
      network_delegate_(new AtomNetworkDelegate),
#endif
      partition_(partition) {
  if (in_memory) {
    original_context_ = static_cast<BraveBrowserContext*>(
        BraveBrowserContext::From(partition, false).get());
    original_context()->otr_context_ = this;
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
  return job_factory;
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
#if defined(ENABLE_EXTENSIONS)
    extensions::AtomBrowserClientExtensionsPart::RegisteryProfilePrefs(
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
  if (!success)
    return;

  BrowserContextDependencyManager::GetInstance()->
      CreateBrowserContextServices(this);

  if (!IsOffTheRecord()) {
#if defined(ENABLE_EXTENSIONS)
    if (extensions::ExtensionsBrowserClient::Get()) {
      extensions::ExtensionSystem::Get(this)->InitForRegularProfile(true);
    }
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

}  // namespace brave

namespace brightray {

// TODO(bridiver) find a better way to do this
// static
scoped_refptr<BrowserContext> BrowserContext::Create(
    const std::string& partition, bool in_memory) {
  return make_scoped_refptr(new brave::BraveBrowserContext(partition, in_memory));
}

}  // namespace brightray

