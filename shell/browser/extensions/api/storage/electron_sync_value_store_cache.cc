// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/storage/electron_sync_value_store_cache.h"

#include <utility>

#include "components/value_store/value_store.h"
#include "components/value_store/value_store_factory.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/storage/backend_task_runner.h"
#include "extensions/browser/api/storage/settings_namespace.h"
#include "extensions/browser/api/storage/value_store_util.h"
#include "extensions/common/api/storage.h"
#include "extensions/common/extension.h"

namespace extensions {

namespace {

// Returns the quota limits for sync storage, taken from the schema in
// extensions/common/api/storage.json.
SettingsStorageQuotaEnforcer::Limits GetSyncQuotaLimits() {
  return {static_cast<size_t>(api::storage::sync::QUOTA_BYTES),
          static_cast<size_t>(api::storage::sync::QUOTA_BYTES_PER_ITEM),
          static_cast<size_t>(api::storage::sync::MAX_ITEMS)};
}

}  // namespace

ElectronSyncValueStoreCache::ElectronSyncValueStoreCache(
    scoped_refptr<value_store::ValueStoreFactory> factory)
    : storage_factory_(std::move(factory)), quota_(GetSyncQuotaLimits()) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

ElectronSyncValueStoreCache::~ElectronSyncValueStoreCache() {
  DCHECK(IsOnBackendSequence());
}

void ElectronSyncValueStoreCache::RunWithValueStoreForExtension(
    StorageCallback callback,
    scoped_refptr<const Extension> extension) {
  DCHECK(IsOnBackendSequence());
  // Unlike LOCAL, the unlimitedStorage permission does not lift the sync
  // quotas, so the enforcer is always handed out directly.
  std::move(callback).Run(GetStorage(extension.get()));
}

void ElectronSyncValueStoreCache::DeleteStorageSoon(
    const ExtensionId& extension_id) {
  DCHECK(IsOnBackendSequence());
  storage_map_.erase(extension_id);

  value_store_util::DeleteValueStore(settings_namespace::SYNC,
                                     value_store_util::ModelType::APP,
                                     extension_id, storage_factory_);

  value_store_util::DeleteValueStore(settings_namespace::SYNC,
                                     value_store_util::ModelType::EXTENSION,
                                     extension_id, storage_factory_);
}

value_store::ValueStore* ElectronSyncValueStoreCache::GetStorage(
    const Extension* extension) {
  auto iter = storage_map_.find(extension->id());
  if (iter != storage_map_.end())
    return iter->second.get();

  value_store_util::ModelType model_type =
      extension->is_app() ? value_store_util::ModelType::APP
                          : value_store_util::ModelType::EXTENSION;
  std::unique_ptr<value_store::ValueStore> store =
      value_store_util::CreateSettingsStore(settings_namespace::SYNC,
                                            model_type, extension->id(),
                                            storage_factory_);
  auto storage =
      std::make_unique<SettingsStorageQuotaEnforcer>(quota_, std::move(store));
  DCHECK(storage);

  value_store::ValueStore* storage_ptr = storage.get();
  storage_map_[extension->id()] = std::move(storage);
  return storage_ptr;
}

}  // namespace extensions
