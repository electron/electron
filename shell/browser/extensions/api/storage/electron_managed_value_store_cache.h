// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_API_STORAGE_ELECTRON_MANAGED_VALUE_STORE_CACHE_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_API_STORAGE_ELECTRON_MANAGED_VALUE_STORE_CACHE_H_

#include <memory>

#include "extensions/browser/api/storage/value_store_cache.h"
#include "extensions/common/extension_id.h"

namespace value_store {
class ValueStore;
}  // namespace value_store

namespace extensions {

// ValueStoreCache for the MANAGED namespace. Electron has no enterprise policy
// service, so every extension sees the same empty read-only store, which
// matches Chrome when no policy is set for the extension.
class ElectronManagedValueStoreCache : public ValueStoreCache {
 public:
  ElectronManagedValueStoreCache();
  ~ElectronManagedValueStoreCache() override;

  // disable copy
  ElectronManagedValueStoreCache(const ElectronManagedValueStoreCache&) =
      delete;
  ElectronManagedValueStoreCache& operator=(
      const ElectronManagedValueStoreCache&) = delete;

  // ValueStoreCache:
  void RunWithValueStoreForExtension(
      StorageCallback callback,
      scoped_refptr<const Extension> extension) override;
  void DeleteStorageSoon(const ExtensionId& extension_id) override;

 private:
  const std::unique_ptr<value_store::ValueStore> store_;
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_API_STORAGE_ELECTRON_MANAGED_VALUE_STORE_CACHE_H_
