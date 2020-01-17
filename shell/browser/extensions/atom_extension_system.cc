// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/atom_extension_system.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/api/app_runtime/app_runtime_api.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/null_app_sorting.h"
#include "extensions/browser/quota_service.h"
#include "extensions/browser/runtime_data.h"
#include "extensions/browser/service_worker_manager.h"
#include "extensions/browser/shared_user_script_master.h"
#include "extensions/browser/value_store/value_store_factory_impl.h"
#include "extensions/common/constants.h"
#include "extensions/common/file_util.h"
#include "shell/browser/extensions/atom_extension_loader.h"

using content::BrowserContext;
using content::BrowserThread;

namespace extensions {

AtomExtensionSystem::AtomExtensionSystem(BrowserContext* browser_context)
    : browser_context_(browser_context),
      store_factory_(new ValueStoreFactoryImpl(browser_context->GetPath())),
      weak_factory_(this) {}

AtomExtensionSystem::~AtomExtensionSystem() = default;

void AtomExtensionSystem::LoadExtension(
    const base::FilePath& extension_dir,
    base::OnceCallback<void(const Extension*)> loaded) {
  extension_loader_->LoadExtension(extension_dir, std::move(loaded));
}

void AtomExtensionSystem::FinishInitialization() {
  // Inform the rest of the extensions system to start.
  ready_.Signal();
  content::NotificationService::current()->Notify(
      NOTIFICATION_EXTENSIONS_READY_DEPRECATED,
      content::Source<BrowserContext>(browser_context_),
      content::NotificationService::NoDetails());
}

void AtomExtensionSystem::ReloadExtension(const ExtensionId& extension_id) {
  extension_loader_->ReloadExtension(extension_id);
}

void AtomExtensionSystem::RemoveExtension(const ExtensionId& extension_id) {
  extension_loader_->UnloadExtension(
      extension_id, extensions::UnloadedExtensionReason::UNINSTALL);
}

void AtomExtensionSystem::Shutdown() {
  extension_loader_.reset();
}

void AtomExtensionSystem::InitForRegularProfile(bool extensions_enabled) {
  service_worker_manager_ =
      std::make_unique<ServiceWorkerManager>(browser_context_);
  runtime_data_ =
      std::make_unique<RuntimeData>(ExtensionRegistry::Get(browser_context_));
  quota_service_ = std::make_unique<QuotaService>();
  shared_user_script_master_ =
      std::make_unique<SharedUserScriptMaster>(browser_context_);
  app_sorting_ = std::make_unique<NullAppSorting>();
  extension_loader_ = std::make_unique<AtomExtensionLoader>(browser_context_);
}

ExtensionService* AtomExtensionSystem::extension_service() {
  return nullptr;
}

RuntimeData* AtomExtensionSystem::runtime_data() {
  return runtime_data_.get();
}

ManagementPolicy* AtomExtensionSystem::management_policy() {
  return nullptr;
}

ServiceWorkerManager* AtomExtensionSystem::service_worker_manager() {
  return service_worker_manager_.get();
}

SharedUserScriptMaster* AtomExtensionSystem::shared_user_script_master() {
  return new SharedUserScriptMaster(browser_context_);
}

StateStore* AtomExtensionSystem::state_store() {
  return nullptr;
}

StateStore* AtomExtensionSystem::rules_store() {
  return nullptr;
}

scoped_refptr<ValueStoreFactory> AtomExtensionSystem::store_factory() {
  return store_factory_;
}

InfoMap* AtomExtensionSystem::info_map() {
  if (!info_map_.get())
    info_map_ = new InfoMap;
  return info_map_.get();
}

QuotaService* AtomExtensionSystem::quota_service() {
  return quota_service_.get();
}

AppSorting* AtomExtensionSystem::app_sorting() {
  return app_sorting_.get();
}

void AtomExtensionSystem::RegisterExtensionWithRequestContexts(
    const Extension* extension,
    const base::Closure& callback) {
  base::PostTaskAndReply(
      FROM_HERE, {BrowserThread::IO},
      base::Bind(&InfoMap::AddExtension, info_map(),
                 base::RetainedRef(extension), base::Time::Now(), false, false),
      callback);
}

void AtomExtensionSystem::UnregisterExtensionWithRequestContexts(
    const std::string& extension_id,
    const UnloadedExtensionReason reason) {}

const base::OneShotEvent& AtomExtensionSystem::ready() const {
  return ready_;
}

ContentVerifier* AtomExtensionSystem::content_verifier() {
  return nullptr;
}

std::unique_ptr<ExtensionSet> AtomExtensionSystem::GetDependentExtensions(
    const Extension* extension) {
  return std::make_unique<ExtensionSet>();
}

void AtomExtensionSystem::InstallUpdate(
    const std::string& extension_id,
    const std::string& public_key,
    const base::FilePath& temp_dir,
    bool install_immediately,
    InstallUpdateCallback install_update_callback) {
  NOTREACHED();
  base::DeleteFile(temp_dir, true /* recursive */);
}

bool AtomExtensionSystem::FinishDelayedInstallationIfReady(
    const std::string& extension_id,
    bool install_immediately) {
  NOTREACHED();
  return false;
}

void AtomExtensionSystem::OnExtensionRegisteredWithRequestContexts(
    scoped_refptr<Extension> extension) {
  ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context_);
  registry->AddReady(extension);
  registry->TriggerOnReady(extension.get());
}

}  // namespace extensions
