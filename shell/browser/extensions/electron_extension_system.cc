// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extension_system.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "electron/buildflags/buildflags.h"
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
#include "shell/browser/extensions/electron_extension_loader.h"

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "chrome/browser/pdf/pdf_extension_util.h"
#endif

using content::BrowserContext;
using content::BrowserThread;

namespace extensions {

ElectronExtensionSystem::ElectronExtensionSystem(
    BrowserContext* browser_context)
    : browser_context_(browser_context),
      store_factory_(new ValueStoreFactoryImpl(browser_context->GetPath())),
      weak_factory_(this) {}

ElectronExtensionSystem::~ElectronExtensionSystem() = default;

void ElectronExtensionSystem::LoadExtension(
    const base::FilePath& extension_dir,
    base::OnceCallback<void(const Extension*, const std::string&)> cb) {
  extension_loader_->LoadExtension(extension_dir, std::move(cb));
}

void ElectronExtensionSystem::FinishInitialization() {
  // Inform the rest of the extensions system to start.
  ready_.Signal();
}

void ElectronExtensionSystem::ReloadExtension(const ExtensionId& extension_id) {
  extension_loader_->ReloadExtension(extension_id);
}

void ElectronExtensionSystem::RemoveExtension(const ExtensionId& extension_id) {
  extension_loader_->UnloadExtension(
      extension_id, extensions::UnloadedExtensionReason::UNINSTALL);
}

void ElectronExtensionSystem::Shutdown() {
  extension_loader_.reset();
}

void ElectronExtensionSystem::InitForRegularProfile(bool extensions_enabled) {
  service_worker_manager_ =
      std::make_unique<ServiceWorkerManager>(browser_context_);
  runtime_data_ =
      std::make_unique<RuntimeData>(ExtensionRegistry::Get(browser_context_));
  quota_service_ = std::make_unique<QuotaService>();
  shared_user_script_master_ =
      std::make_unique<SharedUserScriptMaster>(browser_context_);
  app_sorting_ = std::make_unique<NullAppSorting>();
  extension_loader_ =
      std::make_unique<ElectronExtensionLoader>(browser_context_);

  if (!browser_context_->IsOffTheRecord())
    LoadComponentExtensions();
}

std::unique_ptr<base::DictionaryValue> ParseManifest(
    base::StringPiece manifest_contents) {
  JSONStringValueDeserializer deserializer(manifest_contents);
  std::unique_ptr<base::Value> manifest = deserializer.Deserialize(NULL, NULL);

  if (!manifest.get() || !manifest->is_dict()) {
    LOG(ERROR) << "Failed to parse extension manifest.";
    return std::unique_ptr<base::DictionaryValue>();
  }
  return base::DictionaryValue::From(std::move(manifest));
}

void ElectronExtensionSystem::LoadComponentExtensions() {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  std::string utf8_error;
  std::string pdf_manifest_string = pdf_extension_util::GetManifest();
  std::unique_ptr<base::DictionaryValue> pdf_manifest =
      ParseManifest(pdf_manifest_string);
  base::FilePath root_directory;
  CHECK(base::PathService::Get(chrome::DIR_RESOURCES, &root_directory));
  root_directory = root_directory.Append(FILE_PATH_LITERAL("pdf"));
  scoped_refptr<const Extension> pdf_extension = extensions::Extension::Create(
      root_directory, extensions::Manifest::COMPONENT, *pdf_manifest,
      extensions::Extension::REQUIRE_KEY, &utf8_error);
  extension_loader_->registrar()->AddExtension(pdf_extension);
#endif
}

ExtensionService* ElectronExtensionSystem::extension_service() {
  return nullptr;
}

RuntimeData* ElectronExtensionSystem::runtime_data() {
  return runtime_data_.get();
}

ManagementPolicy* ElectronExtensionSystem::management_policy() {
  return nullptr;
}

ServiceWorkerManager* ElectronExtensionSystem::service_worker_manager() {
  return service_worker_manager_.get();
}

SharedUserScriptMaster* ElectronExtensionSystem::shared_user_script_master() {
  return new SharedUserScriptMaster(browser_context_);
}

StateStore* ElectronExtensionSystem::state_store() {
  return nullptr;
}

StateStore* ElectronExtensionSystem::rules_store() {
  return nullptr;
}

scoped_refptr<ValueStoreFactory> ElectronExtensionSystem::store_factory() {
  return store_factory_;
}

InfoMap* ElectronExtensionSystem::info_map() {
  if (!info_map_.get())
    info_map_ = new InfoMap;
  return info_map_.get();
}

QuotaService* ElectronExtensionSystem::quota_service() {
  return quota_service_.get();
}

AppSorting* ElectronExtensionSystem::app_sorting() {
  return app_sorting_.get();
}

void ElectronExtensionSystem::RegisterExtensionWithRequestContexts(
    const Extension* extension,
    base::OnceClosure callback) {
  base::PostTaskAndReply(
      FROM_HERE, {BrowserThread::IO},
      base::Bind(&InfoMap::AddExtension, info_map(),
                 base::RetainedRef(extension), base::Time::Now(), false, false),
      std::move(callback));
}

void ElectronExtensionSystem::UnregisterExtensionWithRequestContexts(
    const std::string& extension_id,
    const UnloadedExtensionReason reason) {}

const base::OneShotEvent& ElectronExtensionSystem::ready() const {
  return ready_;
}

ContentVerifier* ElectronExtensionSystem::content_verifier() {
  return nullptr;
}

std::unique_ptr<ExtensionSet> ElectronExtensionSystem::GetDependentExtensions(
    const Extension* extension) {
  return std::make_unique<ExtensionSet>();
}

void ElectronExtensionSystem::InstallUpdate(
    const std::string& extension_id,
    const std::string& public_key,
    const base::FilePath& temp_dir,
    bool install_immediately,
    InstallUpdateCallback install_update_callback) {
  NOTREACHED();
  base::DeleteFile(temp_dir, true /* recursive */);
}

bool ElectronExtensionSystem::FinishDelayedInstallationIfReady(
    const std::string& extension_id,
    bool install_immediately) {
  NOTREACHED();
  return false;
}

void ElectronExtensionSystem::PerformActionBasedOnOmahaAttributes(
    const std::string& extension_id,
    const base::Value& attributes) {
  NOTREACHED();
}

void ElectronExtensionSystem::OnExtensionRegisteredWithRequestContexts(
    scoped_refptr<Extension> extension) {
  ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context_);
  registry->AddReady(extension);
  registry->TriggerOnReady(extension.get());
}

}  // namespace extensions
