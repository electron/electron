// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_API_STORAGE_ELECTRON_SYNC_VALUE_STORE_CACHE_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_API_STORAGE_ELECTRON_SYNC_VALUE_STORE_CACHE_H_

#include <map>
#include <memory>

#include "base/memory/scoped_refptr.h"
#include "extensions/browser/api/storage/settings_storage_quota_enforcer.h"
#include "extensions/browser/api/storage/value_store_cache.h"
#include "extensions/common/extension_id.h"

namespace value_store {
class ValueStore;
class ValueStoreFactory;
}  // namespace value_store

namespace extensions {

// ValueStoreCache for the SYNC namespace. Electron has no sync backend, so
// this behaves like Chrome with sync disabled: a second on-disk storage area,
// separate from LOCAL, that enforces the chrome.storage.sync quotas but never
// leaves the device.
class ElectronSyncValueStoreCache : public ValueStoreCache {
 public:
  explicit ElectronSyncValueStoreCache(
      scoped_refptr<value_store::ValueStoreFactory> factory);

  ElectronSyncValueStoreCache(const ElectronSyncValueStoreCache&) = delete;
  ElectronSyncValueStoreCache& operator=(const ElectronSyncValueStoreCache&) =
      delete;

  ~ElectronSyncValueStoreCache() override;

  // ValueStoreCache implementation:
  void RunWithValueStoreForExtension(
      StorageCallback callback,
      scoped_refptr<const Extension> extension) override;
  void DeleteStorageSoon(const ExtensionId& extension_id) override;

 private:
  using StorageMap =
      std::map<ExtensionId, std::unique_ptr<value_store::ValueStore>>;

  value_store::ValueStore* GetStorage(const Extension* extension);

  // The factory to use for creating new ValueStores.
  const scoped_refptr<value_store::ValueStoreFactory> storage_factory_;

  // Quota limits (see SettingsStorageQuotaEnforcer).
  const SettingsStorageQuotaEnforcer::Limits quota_;

  // The collection of ValueStores for sync storage.
  StorageMap storage_map_;
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_API_STORAGE_ELECTRON_SYNC_VALUE_STORE_CACHE_H_
