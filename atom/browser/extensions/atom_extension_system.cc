// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extension_system.h"

#include <set>
#include <string>
#include <vector>
#include "atom/browser/api/atom_api_extension.h"
#include "atom/browser/extensions/atom_extension_system_factory.h"
#include "atom/browser/extensions/atom_extensions_browser_client.h"
#include "atom/browser/extensions/atom_notification_types.h"
#include "atom/browser/extensions/shared_user_script_master.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/management_policy.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/null_app_sorting.h"
#include "extensions/browser/quota_service.h"
#include "extensions/browser/runtime_data.h"
#include "extensions/browser/service_worker_manager.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/manifest_handlers/shared_module_info.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/browser/value_store/value_store_factory_impl.h"

using content::BrowserThread;

namespace extensions {

//
// AtomExtensionSystem::Shared
//

AtomExtensionSystem::Shared::Shared(atom::AtomBrowserContext* browser_context)
    : registry_(ExtensionRegistry::Get(browser_context)),
      browser_context_(browser_context),
      extension_prefs_(ExtensionPrefs::Get(browser_context_)) {
}

AtomExtensionSystem::Shared::~Shared() {
}

void AtomExtensionSystem::Shared::InitPrefs() {
  store_factory_ = new ValueStoreFactoryImpl(browser_context_->GetPath());
}

void AtomExtensionSystem::Shared::Init(bool extensions_enabled) {
  service_worker_manager_.reset(new ServiceWorkerManager(browser_context_));
  runtime_data_.reset(
      new RuntimeData(ExtensionRegistry::Get(browser_context_)));
  quota_service_.reset(new QuotaService);
  shared_user_script_master_.reset(
                                new SharedUserScriptMaster(browser_context_));
  management_policy_.reset(new ManagementPolicy);

  registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_TERMINATED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this, extensions::NOTIFICATION_CRX_INSTALLER_DONE,
                 content::NotificationService::AllSources());
  registrar_.Add(this, atom::NOTIFICATION_ENABLE_USER_EXTENSION_REQUEST,
                 content::NotificationService::AllSources());
  registrar_.Add(this, atom::NOTIFICATION_DISABLE_USER_EXTENSION_REQUEST,
                 content::NotificationService::AllSources());

  if (extensions_enabled) {
    // load all extensions
    const ExtensionSet& extensions =
                            atom::api::Extension::GetInstance()->extensions();

    for (ExtensionSet::const_iterator iter = extensions.begin();
         iter != extensions.end(); ++iter) {
      AddExtension(iter->get());
    }

    ready_.Signal();
    content::NotificationService::current()->Notify(
        extensions::NOTIFICATION_EXTENSIONS_READY_DEPRECATED,
        content::Source<content::BrowserContext>(browser_context_),
        content::NotificationService::NoDetails());
  }
}

void AtomExtensionSystem::Shared::Shutdown() {
}

ExtensionService* AtomExtensionSystem::Shared::extension_service() {
  return is_ready() ? this : nullptr;
}

ServiceWorkerManager* AtomExtensionSystem::Shared::service_worker_manager() {
  return service_worker_manager_.get();
}

scoped_refptr<ValueStoreFactory> AtomExtensionSystem::Shared::store_factory() {
  return store_factory_;
}

StateStore* AtomExtensionSystem::Shared::state_store() {
  return nullptr;
}

StateStore* AtomExtensionSystem::Shared::rules_store() {
  return nullptr;
}

RuntimeData* AtomExtensionSystem::Shared::runtime_data() {
  return runtime_data_.get();
}

ManagementPolicy* AtomExtensionSystem::Shared::management_policy() {
  return management_policy_.get();
}

SharedUserScriptMaster*
AtomExtensionSystem::Shared::shared_user_script_master() {
  return shared_user_script_master_.get();
}

InfoMap* AtomExtensionSystem::Shared::info_map() {
  if (!extension_info_map_.get())
    extension_info_map_ = new InfoMap();
  return extension_info_map_.get();
}

QuotaService* AtomExtensionSystem::Shared::quota_service() {
  return quota_service_.get();
}

AppSorting* AtomExtensionSystem::Shared::app_sorting() {
  // return app_sorting_.get();
  return nullptr;
}

ContentVerifier* AtomExtensionSystem::Shared::content_verifier() {
  return nullptr;
}

void AtomExtensionSystem::Shared::OnExtensionRegisteredWithRequestContexts(
    scoped_refptr<const Extension> extension) {
  registry_->AddReady(extension);
  if (registry_->enabled_extensions().Contains(extension->id()))
    registry_->TriggerOnReady(extension.get());
}

bool AtomExtensionSystem::Shared::IsExtensionEnabled(
    const std::string& extension_id) const {
  if (registry_->enabled_extensions().Contains(extension_id) ||
      registry_->terminated_extensions().Contains(extension_id)) {
    return true;
  }

  return false;
}

const Extension* AtomExtensionSystem::Shared::GetInstalledExtension(
    const std::string& id) const {
  return registry_->GetExtensionById(id, ExtensionRegistry::EVERYTHING);
}

void AtomExtensionSystem::Shared::EnableExtension(
    const std::string& extension_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (IsExtensionEnabled(extension_id))
    return;
  const Extension* extension = GetInstalledExtension(extension_id);

  extension_prefs_->SetExtensionEnabled(extension_id);

  // Move it over to the enabled list.
  registry_->AddEnabled(make_scoped_refptr(extension));
  registry_->RemoveDisabled(extension->id());

  NotifyExtensionLoaded(extension);

  // Notify listeners that the extension was enabled.
  content::NotificationService::current()->Notify(
      extensions::NOTIFICATION_EXTENSION_ENABLED,
      content::Source<content::BrowserContext>(browser_context_),
      content::Details<const Extension>(extension));
}

void AtomExtensionSystem::Shared::DisableExtension(
      const std::string& extension_id, int disable_reasons) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  // The extension may have been disabled already. Just add the disable reasons.
  if (!IsExtensionEnabled(extension_id)) {
    extension_prefs_->AddDisableReasons(extension_id, disable_reasons);
    return;
  }

  const Extension* extension = GetInstalledExtension(extension_id);

  if (extension &&
      !(disable_reasons & Extension::DISABLE_RELOAD) &&
      !(disable_reasons & Extension::DISABLE_UPDATE_REQUIRED_BY_POLICY) &&
      extension->location() != Manifest::EXTERNAL_COMPONENT &&
      extension->location() != Manifest::UNPACKED) {
    return;
  }

  extension_prefs_->SetExtensionDisabled(extension_id, disable_reasons);

  int include_mask =
      ExtensionRegistry::EVERYTHING & ~ExtensionRegistry::DISABLED;
  extension = registry_->GetExtensionById(extension_id, include_mask);
  if (!extension)
    return;

  // The extension is either enabled or terminated.
  DCHECK(registry_->enabled_extensions().Contains(extension->id()) ||
         registry_->terminated_extensions().Contains(extension->id()));

  // Move it over to the disabled list. Don't send a second unload notification
  // for terminated extensions being disabled.
  registry_->AddDisabled(make_scoped_refptr(extension));
  if (registry_->enabled_extensions().Contains(extension->id())) {
    registry_->RemoveEnabled(extension->id());
    NotifyExtensionUnloaded(extension,
        extensions::UnloadedExtensionInfo::REASON_DISABLE);
  } else {
    registry_->RemoveTerminated(extension->id());
  }
}

void AtomExtensionSystem::Shared::NotifyExtensionUnloaded(
    const Extension* extension,
    extensions::UnloadedExtensionInfo::Reason reason) {
  extensions::UnloadedExtensionInfo details(extension, reason);

  registry_->TriggerOnUnloaded(extension, reason);

  content::NotificationService::current()->Notify(
      extensions::NOTIFICATION_EXTENSION_UNLOADED_DEPRECATED,
      content::Source<content::BrowserContext>(browser_context_),
      content::Details<extensions::UnloadedExtensionInfo>(&details));

  for (content::RenderProcessHost::iterator i(
          content::RenderProcessHost::AllHostsIterator());
       !i.IsAtEnd(); i.Advance()) {
    content::RenderProcessHost* host = i.GetCurrentValue();
    if (extensions::ExtensionsBrowserClient::Get()->
        IsSameContext(browser_context_, host->GetBrowserContext()))
      host->Send(new ExtensionMsg_Unloaded(extension->id()));
  }

  AtomExtensionSystemFactory::GetInstance()->
      GetForBrowserContext(browser_context_)->
        UnregisterExtensionWithRequestContexts(extension->id(), reason);
}

const Extension* AtomExtensionSystem::Shared::AddExtension(
                                        const Extension* extension) {
  if (registry_->GetExtensionById(extension->id(),
                                  ExtensionRegistry::IncludeFlag::EVERYTHING))
    return extension;

  extension_prefs_->AddGrantedPermissions(
          extension->id(), extension->permissions_data()->active_permissions());
  extension_prefs_->SetExtensionEnabled(extension->id());

  registry_->AddEnabled(make_scoped_refptr(extension));

  NotifyExtensionLoaded(extension);

  return extension;
}

void AtomExtensionSystem::Shared::NotifyExtensionLoaded(
      const Extension* extension) {
  AtomExtensionSystemFactory::GetInstance()->
      GetForBrowserContext(browser_context_)->
        RegisterExtensionWithRequestContexts(
          extension,
          base::Bind(
        &AtomExtensionSystem::Shared::OnExtensionRegisteredWithRequestContexts,
            AsWeakPtr(), base::RetainedRef(extension)));

  for (content::RenderProcessHost::iterator i(
          content::RenderProcessHost::AllHostsIterator());
       !i.IsAtEnd(); i.Advance()) {
    content::RenderProcessHost* host = i.GetCurrentValue();

    if (extensions::ExtensionsBrowserClient::Get()->
        IsSameContext(browser_context_, host->GetBrowserContext())) {
      std::vector<ExtensionMsg_Loaded_Params> loaded_extensions(
          1, ExtensionMsg_Loaded_Params(extension, false));
      host->Send(
          new ExtensionMsg_Loaded(loaded_extensions));
    }
  }

  // Tell subsystems that use the EXTENSION_LOADED notification about the new
  // extension.
  //
  // NOTE: It is important that this happen after notifying the renderers about
  // the new extensions so that if we navigate to an extension URL in
  // ExtensionRegistryObserver::OnLoaded or
  // NOTIFICATION_EXTENSION_LOADED_DEPRECATED, the
  // renderer is guaranteed to know about it.
  registry_->TriggerOnLoaded(extension);

  content::NotificationService::current()->Notify(
      extensions::NOTIFICATION_EXTENSION_LOADED_DEPRECATED,
      content::Source<content::BrowserContext>(browser_context_),
      content::Details<const Extension>(extension));

  registry_->TriggerOnInstalled(extension, false);
}

const Extension* AtomExtensionSystem::Shared::GetExtensionById(
    const std::string& id,
    bool include_disabled) const {
  return ExtensionRegistry::Get(browser_context_)->
      GetExtensionById(id, include_disabled);
}

bool AtomExtensionSystem::Shared::is_ready() {
  return ready().is_signaled();
}

void AtomExtensionSystem::Shared::Observe(int type,
                               const content::NotificationSource& source,
                               const content::NotificationDetails& details) {
  switch (type) {
    case extensions::NOTIFICATION_CRX_INSTALLER_DONE: {
      if (!shared_user_script_master())
        return;

      const Extension* extension =
        content::Details<const Extension>(details).ptr();

      if (extension)
        AddExtension(extension);

      break;
    }
    case atom::NOTIFICATION_ENABLE_USER_EXTENSION_REQUEST: {
      if (!shared_user_script_master())
        return;

      const Extension* extension =
        content::Details<const Extension>(details).ptr();

      if (extension)
        EnableExtension(extension->id());

      break;
    }
    case atom::NOTIFICATION_DISABLE_USER_EXTENSION_REQUEST: {
      if (!shared_user_script_master())
        return;

      const Extension* extension =
        content::Details<const Extension>(details).ptr();

      if (extension)
        DisableExtension(extension->id(), Extension::DISABLE_USER_ACTION);

      break;
    }
    case content::NOTIFICATION_RENDERER_PROCESS_TERMINATED: {
      content::RenderProcessHost* process =
          content::Source<content::RenderProcessHost>(source).ptr();
      auto host_browser_context = process->GetBrowserContext();

      if (!extensions::ExtensionsBrowserClient::Get()->
              IsSameContext(browser_context_, host_browser_context))
          break;

      extensions::ProcessMap* process_map =
          extensions::ProcessMap::Get(browser_context_);
      if (process_map->Contains(process->GetID())) {
        // An extension process was terminated, this might have resulted in an
        // app or extension becoming idle.
        std::set<std::string> extension_ids =
            process_map->GetExtensionsInProcess(process->GetID());
        // In addition to the extensions listed in the process map, one of those
        // extensions could be referencing a shared module which is waiting for
        // idle to update.  Check all imports of these extensions, too.
        std::set<std::string> import_ids;
        for (std::set<std::string>::const_iterator it = extension_ids.begin();
             it != extension_ids.end();
             ++it) {
          const Extension* extension = GetExtensionById(*it, true);
          if (!extension)
            continue;
          const std::vector<SharedModuleInfo::ImportInfo>& imports =
              SharedModuleInfo::GetImports(extension);
          std::vector<SharedModuleInfo::ImportInfo>::const_iterator import_it;
          for (import_it = imports.begin(); import_it != imports.end();
               import_it++) {
            import_ids.insert((*import_it).extension_id);
          }
        }
        extension_ids.insert(import_ids.begin(), import_ids.end());
      }

      process_map->RemoveAllFromProcess(process->GetID());
      BrowserThread::PostTask(
          BrowserThread::IO,
          FROM_HERE,
          base::Bind(&extensions::InfoMap::UnregisterAllExtensionsInProcess,
                     info_map(),
                     process->GetID()));
      break;
    }
    default:
      NOTREACHED() << "Unexpected notification type.";
  }
}


//
// AtomExtensionSystem
//
AtomExtensionSystem::AtomExtensionSystem(
    atom::AtomBrowserContext* browser_context)
    : browser_context_(browser_context) {
  shared_ =
      AtomExtensionSystemSharedFactory::GetForBrowserContext(browser_context_);

  if (!browser_context_->IsOffTheRecord()) {
    shared_->InitPrefs();
  }
}

AtomExtensionSystem::~AtomExtensionSystem() {
}

void AtomExtensionSystem::Shutdown() {
}

void AtomExtensionSystem::InitForRegularProfile(bool extensions_enabled) {
  DCHECK(!browser_context_->IsOffTheRecord());
  if (shared_user_script_master() || extension_service())
    return;  // Already initialized.

  shared_->info_map();
  shared_->Init(extensions_enabled);
}

ExtensionService* AtomExtensionSystem::extension_service() {
  return shared_->extension_service();
}

RuntimeData* AtomExtensionSystem::runtime_data() {
  return shared_->runtime_data();
}

ManagementPolicy* AtomExtensionSystem::management_policy() {
  return shared_->management_policy();
}

ServiceWorkerManager* AtomExtensionSystem::service_worker_manager() {
  return shared_->service_worker_manager();
}

SharedUserScriptMaster* AtomExtensionSystem::shared_user_script_master() {
  return shared_->shared_user_script_master();
}

StateStore* AtomExtensionSystem::state_store() {
  return shared_->state_store();
}

StateStore* AtomExtensionSystem::rules_store() {
  return shared_->rules_store();
}

scoped_refptr<ValueStoreFactory> AtomExtensionSystem::store_factory() {
  return shared_->store_factory();
}

InfoMap* AtomExtensionSystem::info_map() { return shared_->info_map(); }

const OneShotEvent& AtomExtensionSystem::ready() const {
  return shared_->ready();
}

QuotaService* AtomExtensionSystem::quota_service() {
  return shared_->quota_service();
}

AppSorting* AtomExtensionSystem::app_sorting() {
  return shared_->app_sorting();
}

ContentVerifier* AtomExtensionSystem::content_verifier() {
  return shared_->content_verifier();
}

std::unique_ptr<ExtensionSet> AtomExtensionSystem::GetDependentExtensions(
    const Extension* extension) {
  return base::WrapUnique(new ExtensionSet());
}

void AtomExtensionSystem::InstallUpdate(const std::string& extension_id,
                                         const base::FilePath& temp_dir) {
  NOTREACHED();
}

void AtomExtensionSystem::RegisterExtensionWithRequestContexts(
    const Extension* extension,
    const base::Closure& callback) {
  base::Time install_time;
  bool notifications_disabled = true;
  bool incognito_enabled =
      AtomExtensionsBrowserClient::IsIncognitoEnabled(
        extension->id(), browser_context_);

  BrowserThread::PostTaskAndReply(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&InfoMap::AddExtension, info_map(),
                 extension, install_time, incognito_enabled,
                 notifications_disabled),
      callback);
}

void AtomExtensionSystem::UnregisterExtensionWithRequestContexts(
    const std::string& extension_id,
    const extensions::UnloadedExtensionInfo::Reason reason) {
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&InfoMap::RemoveExtension, info_map(), extension_id, reason));
}

}  // namespace extensions
