// Copyright (c) 2025 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_MAIN_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_MAIN_H_

#include <compare>
#include <string>

#include "base/memory/raw_ptr.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/service_worker_version_base_info.h"
#include "content/public/browser/storage_partition_config.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/pinnable.h"
#include "shell/common/gin_helper/wrappable.h"
#include "third_party/blink/public/common/service_worker/embedded_worker_status.h"

class GURL;

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
class Dictionary;
template <typename T>
class Handle;
template <typename T>
class Promise;
}  // namespace gin_helper

namespace electron::api {

// Key to uniquely identify a ServiceWorkerMain by its
// BrowserContext ID, the StoragePartition key, and version id.
struct ServiceWorkerKey {
  std::string browser_context_id;
  content::StoragePartitionConfig storage_partition_config;
  int64_t version_id;
  auto operator<=>(const ServiceWorkerKey&) const = default;
};

// Creates a wrapper to align with the lifecycle of the non-public
// content::ServiceWorkerVersion. Object instances are pinned for the lifetime
// of the underlying SW such that registered IPC handlers continue to dispatch.
//
// Instances are uniquely identified by pairing their version ID with the
// BrowserContext and StoragePartition in which they're registered.
class ServiceWorkerMain final
    : public gin_helper::DeprecatedWrappable<ServiceWorkerMain>,
      public gin_helper::Pinnable<ServiceWorkerMain>,
      public gin_helper::Constructible<ServiceWorkerMain> {
 public:
  // Create a new ServiceWorkerMain and return the V8 wrapper of it.
  static gin_helper::Handle<ServiceWorkerMain> New(v8::Isolate* isolate);

  static gin_helper::Handle<ServiceWorkerMain> From(
      v8::Isolate* isolate,
      content::ServiceWorkerContext* sw_context,
      std::string browser_context_id,
      content::StoragePartitionConfig storage_partition_config,
      int64_t version_id);
  static ServiceWorkerMain* FromVersionID(
      std::string browser_context_id,
      content::StoragePartitionConfig storage_partition_config,
      int64_t version_id);

  // gin_helper::Constructible
  static void FillObjectTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static const char* GetClassName() { return "ServiceWorkerMain"; }

  // gin_helper::Wrappable
  static gin::DeprecatedWrapperInfo kWrapperInfo;
  const char* GetTypeName() override;

  // disable copy
  ServiceWorkerMain(const ServiceWorkerMain&) = delete;
  ServiceWorkerMain& operator=(const ServiceWorkerMain&) = delete;

  void OnRunningStatusChanged(blink::EmbeddedWorkerStatus running_status);
  void OnVersionRedundant();

 protected:
  explicit ServiceWorkerMain(content::ServiceWorkerContext* sw_context,
                             int64_t version_id,
                             ServiceWorkerKey key);
  ~ServiceWorkerMain() override;

 private:
  void Destroy();
  void MaybeDisconnectRemote();
  const blink::StorageKey GetStorageKey();

  // Increments external requests for the service worker to keep it alive.
  gin_helper::Dictionary StartExternalRequest(v8::Isolate* isolate,
                                              bool has_timeout);
  void FinishExternalRequest(v8::Isolate* isolate, std::string uuid);
  size_t CountExternalRequestsForTest();

  // Get or create a Mojo connection to the renderer process.
  mojom::ElectronRenderer* GetRendererApi();

  // Send a message to the renderer process.
  void Send(v8::Isolate* isolate,
            bool internal,
            const std::string& channel,
            v8::Local<v8::Value> args);

  void InvalidateVersionInfo();
  const content::ServiceWorkerVersionBaseInfo* version_info() const {
    return version_info_.get();
  }

  bool IsDestroyed() const;

  int64_t VersionID() const;
  GURL ScopeURL() const;
  GURL ScriptURL() const;

  // Version ID assigned by the service worker storage.
  const int64_t version_id_;

  // Unique identifier pairing the Version ID, BrowserContext, and
  // StoragePartition.
  const ServiceWorkerKey key_;

  // Whether the Service Worker version has been destroyed.
  bool version_destroyed_ = false;

  // Whether the Service Worker version's state is redundant.
  bool redundant_ = false;

  // Store copy of version info so it's accessible when not running.
  std::unique_ptr<content::ServiceWorkerVersionBaseInfo> version_info_;

  raw_ptr<content::ServiceWorkerContext> service_worker_context_;
  mojo::AssociatedRemote<mojom::ElectronRenderer> remote_;

  std::unique_ptr<gin_helper::Promise<void>> start_worker_promise_;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_MAIN_H_
