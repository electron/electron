// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extension_system.h"

#include <set>
#include <string>
#include <vector>
#include "atom/browser/api/atom_api_extension.h"
#include "atom/browser/extensions/atom_extension_system_factory.h"
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
#include "extensions/browser/notification_types.h"
#include "extensions/browser/null_app_sorting.h"
#include "extensions/browser/quota_service.h"
#include "extensions/browser/runtime_data.h"
#include "extensions/browser/service_worker_manager.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/manifest_handlers/shared_module_info.h"
#include "extensions/common/permissions/permissions_data.h"

using content::BrowserThread;

namespace extensions {

//
// AtomExtensionSystem::Shared
//

AtomExtensionSystem::Shared::Shared(atom::AtomBrowserContext* browser_context)
    : browser_context_(browser_context) {
}

AtomExtensionSystem::Shared::~Shared() {
}

void AtomExtensionSystem::Shared::Init(bool extensions_enabled) {
  service_worker_manager_.reset(new ServiceWorkerManager(browser_context_));
  runtime_data_.reset(
      new RuntimeData(ExtensionRegistry::Get(browser_context_)));
  quota_service_.reset(new QuotaService);
  shared_user_script_master_.reset(
                                new SharedUserScriptMaster(browser_context_));

  // load all extensions
  const ExtensionSet& extensions =
                            atom::api::Extension::GetInstance()->extensions();

  for (ExtensionSet::const_iterator iter = extensions.begin();
       iter != extensions.end(); ++iter) {
    extension_service()->AddExtension(iter->get());
  }

  ready_.Signal();
  content::NotificationService::current()->Notify(
      extensions::NOTIFICATION_EXTENSIONS_READY_DEPRECATED,
      content::Source<content::BrowserContext>(browser_context_),
      content::NotificationService::NoDetails());
}

void AtomExtensionSystem::Shared::Shutdown() {
}

ServiceWorkerManager* AtomExtensionSystem::Shared::service_worker_manager() {
  return service_worker_manager_.get();
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
  return nullptr;
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

//
// AtomExtensionSystem
//
AtomExtensionSystem::AtomExtensionSystem(
    atom::AtomBrowserContext* browser_context)
    : registry_(ExtensionRegistry::Get(browser_context)),
      browser_context_(browser_context) {
  shared_ =
      AtomExtensionSystemSharedFactory::GetForBrowserContext(browser_context_);
}

AtomExtensionSystem::~AtomExtensionSystem() {
}

void AtomExtensionSystem::Shutdown() {
}

void AtomExtensionSystem::InitForRegularProfile(bool extensions_enabled) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (shared_user_script_master())
    return;  // Already initialized.

  shared_->set_extension_service(this);
  shared_->info_map();
  shared_->Init(extensions_enabled);
  registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_TERMINATED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this, extensions::NOTIFICATION_CRX_INSTALLER_DONE,
                 content::NotificationService::AllSources());
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

scoped_ptr<ExtensionSet> AtomExtensionSystem::GetDependentExtensions(
    const Extension* extension) {
  return make_scoped_ptr(new ExtensionSet());
}

void AtomExtensionSystem::RegisterExtensionWithRequestContexts(
    const Extension* extension,
    const base::Closure& callback) {
  base::Time install_time;
  bool incognito_enabled = false;
  bool notifications_disabled = false;

  BrowserThread::PostTaskAndReply(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&InfoMap::AddExtension, info_map(),
                 make_scoped_refptr(extension), install_time, incognito_enabled,
                 notifications_disabled),
      callback);
}

void AtomExtensionSystem::UnregisterExtensionWithRequestContexts(
    const std::string& extension_id,
    const UnloadedExtensionInfo::Reason reason) {
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&InfoMap::RemoveExtension, info_map(), extension_id, reason));
}

void AtomExtensionSystem::OnExtensionRegisteredWithRequestContexts(
    scoped_refptr<const Extension> extension) {
  registry_->AddReady(extension);
  if (registry_->enabled_extensions().Contains(extension->id()))
    registry_->TriggerOnReady(extension.get());
}

const Extension* AtomExtensionSystem::AddExtension(
                                        const Extension* extension) {
  if (registry_->GenerateInstalledExtensionsSet()->Contains(extension->id()))
    return extension;

  ExtensionPrefs::Get(browser_context_)
      ->AddGrantedPermissions(
          extension->id(), extension->permissions_data()->active_permissions());
  ExtensionPrefs::Get(browser_context_)->SetExtensionEnabled(extension->id());

  registry_->AddEnabled(extension);
  registry_->RemoveDisabled(extension->id());

  RegisterExtensionWithRequestContexts(
      extension,
      base::Bind(
          &AtomExtensionSystem::OnExtensionRegisteredWithRequestContexts,
          AsWeakPtr(), make_scoped_refptr(extension)));

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

  return extension;
}

const Extension* AtomExtensionSystem::GetExtensionById(
    const std::string& id,
    bool include_disabled) const {
  return ExtensionRegistry::Get(browser_context_)->
      GetExtensionById(id, include_disabled);
}

void AtomExtensionSystem::ReloadExtension(const std::string& extension_id) {
  // TODO(bridiver) - implement this
}

bool AtomExtensionSystem::is_ready() {
  return ready().is_signaled();
}

void AtomExtensionSystem::InstallUpdate(const std::string& extension_id,
                                         const base::FilePath& temp_dir) {
  NOTREACHED();
}

void AtomExtensionSystem::Observe(int type,
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
}  // namespace extensions
