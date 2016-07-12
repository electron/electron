// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_SYSTEM_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_SYSTEM_H_

#include <string>
#include "atom/browser/atom_browser_context.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_observer.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/one_shot_event.h"

class ExtensionService {
 public:
  virtual bool IsExtensionEnabled(const std::string& extension_id) const = 0;
  virtual const extensions::Extension*
                        GetInstalledExtension(const std::string& id) const = 0;
  virtual void EnableExtension(const std::string& extension_id) = 0;
  virtual void DisableExtension(const std::string& extension_id,
                        int disable_reasons) = 0;
  virtual void NotifyExtensionLoaded(
                                  const extensions::Extension* extension) = 0;
  virtual void NotifyExtensionUnloaded(const extensions::Extension* extension,
                         extensions::UnloadedExtensionInfo::Reason reason) = 0;
  virtual const extensions::Extension* AddExtension(
                                  const extensions::Extension* extension) = 0;
  virtual const extensions::Extension* GetExtensionById(
      const std::string& id,
      bool include_disabled) const = 0;
  virtual bool is_ready() = 0;
};

namespace extensions {

class AtomExtensionSystemSharedFactory;
class AppSorting;
class StateStore;
class ExtensionPrefs;
class ExtensionRegistry;
class ValueStoreFactory;


class AtomExtensionSystem : public ExtensionSystem {
 public:
  explicit AtomExtensionSystem(atom::AtomBrowserContext* browser_context);
  ~AtomExtensionSystem() override;

  // KeyedService implementation.
  void Shutdown() override;

  void InitForRegularProfile(bool extensions_enabled) override;

  // ExtensionSystem implementation;
  ExtensionService* extension_service() override;  // shared
  RuntimeData* runtime_data() override;            // shared
  ManagementPolicy* management_policy() override;  // shared
  ServiceWorkerManager* service_worker_manager() override;  // shared
  SharedUserScriptMaster* shared_user_script_master() override;  // shared
  StateStore* state_store() override;                              // shared
  StateStore* rules_store() override;                              // shared
  scoped_refptr<ValueStoreFactory> store_factory() override;
  InfoMap* info_map() override;                                    // shared
  QuotaService* quota_service() override;  // shared
  AppSorting* app_sorting() override;  // shared
  void RegisterExtensionWithRequestContexts(
      const Extension* extension,
      const base::Closure& callback) override;
  void UnregisterExtensionWithRequestContexts(
      const std::string& extension_id,
      const UnloadedExtensionInfo::Reason reason) override;
  const OneShotEvent& ready() const override;
  ContentVerifier* content_verifier() override;  // shared
  std::unique_ptr<ExtensionSet> GetDependentExtensions(
      const Extension* extension) override;
  void InstallUpdate(const std::string& extension_id,
                      const base::FilePath& temp_dir) override;

 private:
  friend class AtomExtensionSystemSharedFactory;

  class Shared : public KeyedService,
                 public ExtensionService,
                 public content::NotificationObserver,
                 public base::SupportsWeakPtr<Shared> {
   public:
    explicit Shared(atom::AtomBrowserContext* browser_context);
    ~Shared() override;

    void InitPrefs();
    // This must not be called until all the providers have been created.
    void RegisterManagementPolicyProviders();
    void Init(bool extensions_enabled);

    // KeyedService implementation.
    void Shutdown() override;

    scoped_refptr<ValueStoreFactory> store_factory();
    StateStore* state_store();
    StateStore* rules_store();
    ExtensionService* extension_service();
    RuntimeData* runtime_data();
    ManagementPolicy* management_policy();
    ServiceWorkerManager* service_worker_manager();
    SharedUserScriptMaster* shared_user_script_master();
    InfoMap* info_map();
    QuotaService* quota_service();
    AppSorting* app_sorting();
    const OneShotEvent& ready() const { return ready_; }
    ContentVerifier* content_verifier();

    // ExtensionService implementation
    bool IsExtensionEnabled(const std::string& extension_id) const override;
    const Extension* GetInstalledExtension(
                                        const std::string& id) const override;
    void EnableExtension(const std::string& extension_id) override;
    void DisableExtension(const std::string& extension_id,
                          int disable_reasons) override;
    void NotifyExtensionLoaded(const Extension* extension) override;
    void NotifyExtensionUnloaded(const Extension* extension,
                                UnloadedExtensionInfo::Reason reason) override;
    const Extension* AddExtension(const Extension* extension) override;
    const Extension* GetExtensionById(
        const std::string& id,
        bool include_disabled) const override;
    bool is_ready() override;

   private:
    // content::NotificationObserver implementation:
    void Observe(int type,
                 const content::NotificationSource& source,
                 const content::NotificationDetails& details) override;

    void OnExtensionRegisteredWithRequestContexts(
        scoped_refptr<const Extension> extension);

    content::NotificationRegistrar registrar_;

    ExtensionRegistry* registry_;  // Not owned.

    content::BrowserContext* browser_context_;

    extensions::ExtensionPrefs* extension_prefs_;

    scoped_refptr<ValueStoreFactory> store_factory_;

    std::unique_ptr<RuntimeData> runtime_data_;
    std::unique_ptr<QuotaService> quota_service_;
    std::unique_ptr<AppSorting> app_sorting_;

    std::unique_ptr<ServiceWorkerManager> service_worker_manager_;
    // Shared memory region manager for scripts statically declared in extension
    // manifests. This region is shared between all extensions.
    std::unique_ptr<SharedUserScriptMaster> shared_user_script_master_;

    std::unique_ptr<ManagementPolicy> management_policy_;

    // extension_info_map_ needs to outlive process_manager_.
    scoped_refptr<InfoMap> extension_info_map_;

#if defined(OS_CHROMEOS)
    std::unique_ptr<chromeos::DeviceLocalAccountManagementPolicyProvider>
        device_local_account_management_policy_provider_;
#endif

    OneShotEvent ready_;
  };

  content::BrowserContext* browser_context_;

  Shared* shared_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionSystem);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_SYSTEM_H_
