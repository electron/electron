// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/storage/electron_managed_value_store_cache.h"

#include <stddef.h>

#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "components/value_store/value_store.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/storage/backend_task_runner.h"
#include "extensions/common/extension.h"

namespace extensions {

namespace {

using value_store::ValueStore;

ValueStore::Status ReadOnlyError() {
  return ValueStore::Status(ValueStore::READ_ONLY,
                            "This is a read-only store.");
}

// A ValueStore that holds nothing and rejects every write.
class EmptyReadOnlyValueStore : public ValueStore {
 public:
  EmptyReadOnlyValueStore() = default;
  ~EmptyReadOnlyValueStore() override = default;

  // disable copy
  EmptyReadOnlyValueStore(const EmptyReadOnlyValueStore&) = delete;
  EmptyReadOnlyValueStore& operator=(const EmptyReadOnlyValueStore&) = delete;

  // ValueStore:
  size_t GetBytesInUse(const std::string& key) override { return 0; }
  size_t GetBytesInUse(const std::vector<std::string>& keys) override {
    return 0;
  }
  size_t GetBytesInUse() override { return 0; }
  ReadResult GetKeys() override { return EmptyResult(); }
  ReadResult Get(const std::string& key) override { return EmptyResult(); }
  ReadResult Get(const std::vector<std::string>& keys) override {
    return EmptyResult();
  }
  ReadResult Get() override { return EmptyResult(); }
  WriteResult Set(WriteOptions options,
                  const std::string& key,
                  const base::Value& value) override {
    return WriteResult(ReadOnlyError());
  }
  WriteResult Set(WriteOptions options,
                  const base::DictValue& values) override {
    return WriteResult(ReadOnlyError());
  }
  WriteResult Remove(const std::string& key) override {
    return WriteResult(ReadOnlyError());
  }
  WriteResult Remove(const std::vector<std::string>& keys) override {
    return WriteResult(ReadOnlyError());
  }
  WriteResult Clear() override { return WriteResult(ReadOnlyError()); }

 private:
  static ReadResult EmptyResult() {
    return ReadResult(base::DictValue(), Status());
  }
};

}  // namespace

ElectronManagedValueStoreCache::ElectronManagedValueStoreCache()
    : store_(std::make_unique<EmptyReadOnlyValueStore>()) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

ElectronManagedValueStoreCache::~ElectronManagedValueStoreCache() {
  DCHECK(IsOnBackendSequence());
}

void ElectronManagedValueStoreCache::RunWithValueStoreForExtension(
    StorageCallback callback,
    scoped_refptr<const Extension> extension) {
  DCHECK(IsOnBackendSequence());
  std::move(callback).Run(store_.get());
}

void ElectronManagedValueStoreCache::DeleteStorageSoon(
    const ExtensionId& extension_id) {
  DCHECK(IsOnBackendSequence());
}

}  // namespace extensions
